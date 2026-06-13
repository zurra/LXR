// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#include "LXRDetectionComponent.h"
#include "LXRSourceComponent.h"
#include "LXRFunctionLibrary.h"
#include "LXRSubsystem.h"
#include "LXR_Shared.h"
#include "LXR.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/LocalLightComponent.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

#include "Tasks/ProcessDetectionTaskAsyncTask.h"

bool DoCalculation = false;
#if WITH_EDITOR
void ULXRDetectionComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UE_LOG(LogLXR, Warning, TEXT("%s"), *PropertyChangedEvent.GetPropertyName().ToString());

	FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, RelevancyCheckQuality) || PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, VisibilityCheckQuality))
	{
		SetParamsByQualityLevels();
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, SocketPreset))
	{
		if (SocketPreset < ESocketPreset::Custom)
			TargetSockets = *GetPresetSockets(SocketPreset);
	}


	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

// Sets default values for this component's properties
ULXRDetectionComponent::ULXRDetectionComponent() : CombinedLXRColor(), CombinedLXRIntensity(0), AsyncEnabledAtStart(false), LastRelevancyUpdateLocation(), Replicated_DetectedLightIntensity(0),
                                                   Replicated_DetectedLightColor(), CombinedLXRColorTarget(),
                                                   CombinedLXRIntensityTarget(0)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

void ULXRDetectionComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULXRDetectionComponent, Replicated_DetectedLightIntensity)
	DOREPLIFETIME(ULXRDetectionComponent, Replicated_DetectedLightColor)
}

int ULXRDetectionComponent::GetLightToIndex(const TWeakObjectPtr<ULXRSourceComponent> SourceComponent)
{
	return LightToIndex[SourceComponent.Get()];
}

// Called when the game starts
void ULXRDetectionComponent::BeginPlay()
{
	Super::BeginPlay();
	SetIsReplicated(bUserServerAuthority);

	DetectionTaskData = MakeShared<FDetectionTaskData>();
	RelevancyPassedData = MakeShared<FLightSourcePassedData>();
	RelevancyTaskData = MakeShared<FTraceTaskData>();
	DetectionTaskData->Instigator = GetOwner();

	if (GetWorld()->IsPreviewWorld())
	{
		SetComponentTickEnabled(false);
		return;
	}

	DetectionTaskData->DetectionComponent = this;
	DetectionTaskData->TraceChannel = TraceChannel;

	AsyncEnabledAtStart = VisibilityCheckType == ETaskProcessType::Multithread;
	SetComponentTickEnabled(false);

	if (RelevancyTargetType == ETraceTarget::Sockets || VisibilityTargetType == ETraceTarget::Sockets)
	{
		if (SocketPreset < ESocketPreset::Custom)
			TargetSockets = *GetPresetSockets(SocketPreset);

		if (TargetSockets.Num() > 0)
		{
			if (SkeletalMeshComponent.ComponentProperty.IsNone())
				RealSkeletalMeshComponent = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
			else
			{
				RealSkeletalMeshComponent = Cast<USkeletalMeshComponent>(SkeletalMeshComponent.GetComponent(GetOwner()));
			}

			ensureMsgf(RealSkeletalMeshComponent, TEXT("SkeletalMeshComponent does not exists on %s"), *GetOwner()->GetName());

			int Index = 0;
			if (RealSkeletalMeshComponent)
			{
				TArray<FName> Keys;
				TargetSockets.GetKeys(Keys);
				for (FName Socket : Keys)
				{
					if (!RealSkeletalMeshComponent->DoesSocketExist(Socket))
					{
						UE_LOG(LogLXR, Warning, TEXT("Socket %s does not exist on %s"), *Socket.ToString(), *GetOwner()->GetName())
						continue;
					}
					CachedSockets.Add(Socket);
					SocketNameToTraceTarget.Add(Socket);
					// TraceTargetToSocketName.Add(Index, Socket);
					Index++;
				}
			}
		}
	}

	SetParamsByQualityLevels();

	//Lazy "late begin play"
	LXRSubsystem = GetOwner()->GetWorld()->GetSubsystem<ULXRSubsystem>();
	TWeakObjectPtr<ULXRDetectionComponent> WeakSelf(this);
	FTimerHandle LateInitTimer;
	GetWorld()->GetTimerManager().SetTimer(LateInitTimer, FTimerDelegate::CreateLambda([WeakSelf]()
	{
		ULXRDetectionComponent* Self = WeakSelf.Get();
		if (!Self || !IsValid(Self)) return;

		if (Self->RelevancyCheckType == ERelevancyCheckType::Octree)
		{
			if (!Self->LXRSubsystem->GetOctree().IsValid())
			{
				UE_LOG(LogLXR, Error, TEXT("%s RelevancyCheckType == Octree but OctreeActor not found in level"), *Self->GetOwner()->GetName());
				UE_LOG(LogLXR, Error, TEXT("Reverting RelevancyCheckType to Smart for %s"), *Self->GetOwner()->GetName());
				Self->RelevancyCheckType = ERelevancyCheckType::Smart;
			}
		}


		Self->MemoryComponent = Self->GetOwner()->FindComponentByClass<ULXRMemoryComponent>();
		if (IsValid(Self->MemoryComponent))
		{
			Self->DetectionTaskData->IsLightMemoryEnabled = true;
		}

		Self->LXRSubsystem->RegisterDetection(Self);
		Self->GetLightSystemLights();
		if (Self->RelevancyCheckType != ERelevancyCheckType::Octree)
		{
			Self->LXRSubsystem->OnLightAdded.AddUObject(Self, &ULXRDetectionComponent::AddLight);
			Self->LXRSubsystem->OnLightRemoved.AddUObject(Self, &ULXRDetectionComponent::RemoveLight);
		}
		bool bSoloFound = Self->GetWorld()->GetSubsystem<ULXRSubsystem>()->bSoloFound;

		for (int i = 0; i < Self->Entries.Num(); ++i)
		{
			if (auto SourceComponent = Cast<ULXRSourceComponent>(Self->Entries[i].Light))
			{
				if (SourceComponent->bAlwaysRelevant)
				{
					if (bSoloFound)
					{
						if (SourceComponent->bSolo)
						{
							Self->AddRelevantLight(i);
						}
						continue;
					}
					Self->AddRelevantLight(i);
				}
			}
		}


		Self->SetComponentTickEnabled(true);
	}), 0.1f, false);


#if UE_ENABLE_DEBUG_DRAWING
	if (bDebugVectorArray)
	{
		for (FVector& TargetVector : TargetVectors)
		{
			DrawDebugSphere(GetWorld(), GetOwner()->GetActorTransform().TransformPosition(TargetVector), 10, 12, FColor::Green, false, 5.f);
		}
	}
#endif
}

void ULXRDetectionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (GetWorld()->IsPreviewWorld())
	{
		return;
	}

	LXRSubsystem->OnLightAdded.RemoveAll(this);
	LXRSubsystem->OnLightRemoved.RemoveAll(this);
	LXRSubsystem->UnregisterDetection(this);
}

void ULXRDetectionComponent::TickComponent(float DeltaTime, ELevelTick Tick, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, Tick, ThisTickFunction);
	if (!IsValid(GetOwner())) return;

	if (DetectionTaskData == nullptr) return;

	if (!bUserServerAuthority)
	{
		DoCalculation = true;
	}
	else
	{
		if (bUseClientSideCalculation)
		{
			DoCalculation = true;
		}
		else if (GetOwner()->HasAuthority())
		{
			DoCalculation = true;
		}
	}

	if (DoCalculation)
	{
		switch (DetectionTaskData->GetTaskState())
		{
		case ETaskState::Cleanup:
			{
				bool Clear = true;
				for (auto& LightEntry : Entries)
				{
					if (LightEntry.bAlive)
					{
						Clear = false;
						break;
					}
				}
				if (Clear)
				{
					Entries.Reset();
				}

				RemoveStaleLights();
				ProcessPendingEntries();
				CheckAllLightForRelevancy();
				DetectionTaskData->SetState(ETaskState::Processing); // processing
				break;
			}
		case ETaskState::GeneratingTargets:
			break;
		case ETaskState::Processing:
			ProcessDetectionTask();
			break;
		case ETaskState::Analyze:
			if (!DetectionTaskData->IsStateStarted())
			{
				GetLXR();
				DetectionTaskData->NextState();
			}

			break;
		case ETaskState::Max:
			break;
		}
	}
	else
	{
		if (bUserServerAuthority && !GetOwner()->HasAuthority())
		{
			Client_InterpolateCombinedColorTowardsReplicatedColor();
			Client_InterpolateCombinedIntensityTowardsReplicatedIntensity();
		}
	}
}

void ULXRDetectionComponent::ProcessPendingEntries()
{
	for (int32 i = Pending.Num() - 1; i >= 0; --i)
	{
		const int32 Index = Pending[i];
		Pending.RemoveAtSwap(i);

		if (!Entries.IsValidIndex(Index) || !Entries[Index].bAlive)
			continue;

		FLightEntry& Entry = Entries[Index];

		if (EnumHasAnyFlags(Entry.Flags, ELightFlags::PendingRemove))
		{
			FreeEntry(Index);
			Entry.Flags &= ~ELightFlags::PendingRemove;

			continue;
		}

		if (EnumHasAnyFlags(Entry.Flags, ELightFlags::PendingMove))
		{
			MoveEntryToBucket(Entry.PendingBucket, Index);
			Entry.Flags &= ~ELightFlags::PendingMove;
			Entry.PendingBucket = ELightBucket::None;
		}

		if (EnumHasAnyFlags(Entry.Flags, ELightFlags::PendingAdd))
		{
			AssignPendingToValidBucket(Index);
			Entry.Flags &= ~ELightFlags::PendingAdd;
		}
	}
}

void ULXRDetectionComponent::AssignPendingToValidBucket(int EntryIndex)
{
	switch (RelevancyCheckType)
	{
	case ERelevancyCheckType::Fixed:
		MoveEntryToBucket(ELightBucket::All, EntryIndex);
		break;
	case ERelevancyCheckType::Octree:
		MoveEntryToBucket(ELightBucket::Octree, EntryIndex);
		break;
	case ERelevancyCheckType::Smart:
		{
			const ELightBucket BucketType =
				GetSmartBucketForLightFromSqrDistance(FVector::DistSquared(Entries[EntryIndex].Light.Get()->GetOwner()->GetActorLocation(), GetOwner()->GetActorLocation()));
			MoveEntryToBucket(BucketType, EntryIndex);

			break;
		}
	default: ;
	}
}

void ULXRDetectionComponent::OnRep_Replicated_DetectedLightIntensity()
{
	if (bUserServerAuthority && bUseClientSideCalculation)
	{
		CombinedLXRIntensity = Replicated_DetectedLightIntensity;
	}
}

void ULXRDetectionComponent::OnRep_Replicated_DetectedLightColor()
{
	if (bUserServerAuthority && bUseClientSideCalculation)
	{
		CombinedLXRColor = Replicated_DetectedLightColor;
	}
}

void ULXRDetectionComponent::ProcessDetectionTask()
{
	if (DetectionTaskData->TargetIterator == INDEX_NONE || DetectionTaskData->TracePoints.Num() == 0)
	{
		DetectionTaskFinished();
		return;
	}
	if (!DetectionTaskData->IsStateStarted())
		DetectionTaskData->StartState();

	DetectionTaskData->DecreaseSourcesRefTimer();

	switch (VisibilityCheckType)
	{
	case ETaskProcessType::Sync:
		DetectionTaskData->IsBackgroundThread = false;
		(new FAutoDeleteAsyncTask<FProcessDetectionTaskAsyncTask>(DetectionTaskData))->StartSynchronousTask();

		break;
	case ETaskProcessType::Multithread:
		DetectionTaskData->IsBackgroundThread = true;
		(new FAutoDeleteAsyncTask<FProcessDetectionTaskAsyncTask>(DetectionTaskData))->StartBackgroundTask();
		break;
	}
}

void ULXRDetectionComponent::DetectionTaskFinished()
{
	for (auto& Element : DetectionTaskData->PassedDatas)
	{
		if (int32* FoundIndex = LightToIndex.Find(Element.SourceComponent.Get()))
		{
			int32 EntryIndex = *FoundIndex;
			if (!DetectionTaskData->IsSourceStillRelevant(EntryIndex))
			{
				RemoveRelevantLight(EntryIndex);
			}

			else if (Element.SourceComponent->bAddDetected && bAddToSourceWhenDetected)
			{
				if (Element.SourceComponent->DetectedActors.Contains(GetOwner()))
				{
					Element.SourceComponent->DetectedActors.RemoveSwap(GetOwner());
				}
			}
		}
	}

	DetectionTaskData->TracePoints.Reset();
	DetectionTaskData->TracePoints = GetTraceTargets(true);
	DetectionTaskData->TargetIterator = DetectionTaskData->TracePoints.Num() - 1;
	DetectionTaskData->TaskLock = false;
	DetectionTaskData->NextState(); //analyze
}

int ULXRDetectionComponent::GetCurrentBucketIndexByBucketType(const ELightBucket LightBucket) const
{
	switch (LightBucket)
	{
	case ELightBucket::All:
	case ELightBucket::Octree:

		return RelevancyLightIndex;

	case ELightBucket::SmartNear:
		return SmartNearLightIndex;

	case ELightBucket::SmartMid:
		return SmartMidLightIndex;

	case ELightBucket::SmartFar:
		return SmartFarLightIndex;

	case ELightBucket::Relevant:
		return RelevantLightIndex;

	case ELightBucket::Irrelevant:
		break;
	case ELightBucket::None:
		break;
	}
	return -1;
}


void ULXRDetectionComponent::SetCurrentLightArrayIndexByLightBucketType(int InIndex, const ELightBucket LightBucketType)
{
	switch (LightBucketType)
	{
	case ELightBucket::All:
		break;
	case ELightBucket::SmartNear:
		SmartNearLightIndex = InIndex;
		break;
	case ELightBucket::SmartMid:
		SmartMidLightIndex = InIndex;
		break;
	case ELightBucket::SmartFar:
		SmartFarLightIndex = InIndex;
		break;
	case ELightBucket::Octree:
		RelevancyLightIndex = InIndex;
		break;
	case ELightBucket::Relevant:
		RelevantLightIndex = InIndex;
		break;
	case ELightBucket::Irrelevant:
		break;
	case ELightBucket::None:
		break;
	}
}


ELightBucket ULXRDetectionComponent::GetSmartBucketForLight(const FVector& Start, const FVector& End) const
{
	const float DistSqr = FVector::DistSquared(Start, End);
	return GetSmartBucketForLightFromSqrDistance(DistSqr);
}

ELightBucket ULXRDetectionComponent::GetSmartBucketForLightFromSqrDistance(const float& SqrDist) const
{
	if (SqrDist > RelevancySmartDistanceMax * RelevancySmartDistanceMax)
		return ELightBucket::SmartFar;
	if (SqrDist > RelevancySmartDistanceMin * RelevancySmartDistanceMin && SqrDist < RelevancySmartDistanceMax * RelevancySmartDistanceMax)
		return ELightBucket::SmartMid;

	return ELightBucket::SmartNear;
}


FCollisionQueryParams ULXRDetectionComponent::GetCollisionQueryParams(const TArray<AActor*>& ActorsToIgnore) const
{
	FCollisionQueryParams Params(GetOwner()->GetFName(), SCENE_QUERY_STAT_ONLY(KismetTraceUtils), false);

	Params.AddIgnoredActors(ActorsToIgnore);
	return Params;
}


void ULXRDetectionComponent::MoveEntryToBucket(const ELightBucket& NewBucket, int EntryIndex)
{
	FLightEntry& Entry = Entries[EntryIndex];
	if (NewBucket == Entry.Bucket) return;

	// remove from old bucket
	if (Entry.Bucket != ELightBucket::None)
	{
		SwapRemoveFromBucket(EntryIndex);
	}

	if (NewBucket == ELightBucket::Irrelevant)
	{
		AssignPendingToValidBucket(EntryIndex);
		return;
	}

	// add to new bucket
	Entry.Bucket = NewBucket;
	Entry.BucketPos = Buckets[static_cast<uint8>(Entry.Bucket)].Add(EntryIndex);
}

void ULXRDetectionComponent::AddRelevantLight(int EntryIndex)
{
	FLightEntry& Entry = Entries[EntryIndex];
	Entry.Flags |= ELightFlags::PendingMove;
	Entry.PendingBucket = ELightBucket::Relevant;
	Pending.Add(EntryIndex);

	FLightEntry* EntryPtr = nullptr;
	ULXRSourceComponent* Light = GetValidLight(EntryIndex, EntryPtr);
	UE_LOG(LogLXR, VeryVerbose, TEXT("Added relevant light %s to %s"), *Light->GetOwner()->GetName(), *GetOwner()->GetName());
}

void ULXRDetectionComponent::RemoveRelevantLight(int EntryIndex)
{
	FLightEntry& Entry = Entries[EntryIndex];
	const TWeakObjectPtr<ULXRSourceComponent> SourceComponent = Entry.Light;

	Entry.Flags |= ELightFlags::PendingMove;
	Entry.PendingBucket = ELightBucket::Irrelevant;
	Pending.Add(EntryIndex);

	if (SourceComponent->bAddDetected && bAddToSourceWhenDetected)
	{
		if (SourceComponent->DetectedActors.Contains(GetOwner()))
		{
			SourceComponent->DetectedActors.RemoveSwap(GetOwner());
		}
	}
}


void ULXRDetectionComponent::SwapRemoveFromBucket(int EntryIndex)
{
	// remove from old bucket
	FLightEntry& Entry = Entries[EntryIndex];
	if (Entry.Bucket == ELightBucket::None)
	{
		UE_LOG(LogLXR, VeryVerbose, TEXT("as far as I know, this should never happen.." ));
		return;
	}

	const uint8 EntryOldBucket = static_cast<uint8>(Entry.Bucket);
	auto& OldBucket = Buckets[EntryOldBucket];
	const int32 BucketLastPos = OldBucket.Num() - 1;
	const int32 MovedEntryIndex = OldBucket[BucketLastPos];

	OldBucket[Entry.BucketPos] = MovedEntryIndex;
	OldBucket.Pop();

	if (MovedEntryIndex != EntryIndex)
	{
		Entries[MovedEntryIndex].BucketPos = Entry.BucketPos;
	}
	Entry.Bucket = ELightBucket::None;
}

void ULXRDetectionComponent::FreeEntry(int EntryIndex)
{
	SwapRemoveFromBucket(EntryIndex);
	LightToIndex.Remove(Entries[EntryIndex].Light.Get());

	// clear entry and push to freelist
	Entries[EntryIndex] = FLightEntry();
	Entries[EntryIndex].bAlive = false;
	FreeSlots.Add(EntryIndex);
}

void ULXRDetectionComponent::CheckAllLightForRelevancy()
{
	if (bStop) return;

	SCOPE_CYCLE_COUNTER(STAT_RelevancyCheck);

	TArray<int> StaleLightsIndexes;
	TArray<int32> LightBucketBatch;

	switch (RelevancyCheckType)
	{
	case ERelevancyCheckType::Fixed:
		{
			if (!Entries.IsValidIndex(RelevancyLightIndex))
				RelevancyLightIndex = 0;

			GetNextBucketBatch(LightBucketBatch, ELightBucket::All);
			ProcessRelevancyCheckLightBucketBatch(LightBucketBatch, ELightBucket::All);
			break;
		}

	case ERelevancyCheckType::Smart:
		{
			NearSmartTimer += RelevancyCheckRate;
			MidSmartTimer += RelevancyCheckRate;
			FarSmartTimer += RelevancyCheckRate;

			if (FarSmartTimer > 1 / RelevancySmartCheckRateDivider)
			{
				if (SmartFarBucket.Num() > 0)
				{
					GetNextBucketBatch(LightBucketBatch, ELightBucket::SmartFar);
					ProcessRelevancyCheckLightBucketBatch(LightBucketBatch, ELightBucket::SmartFar);
				}
				FarSmartTimer = 0;
			}

			if (MidSmartTimer > 0.5 / RelevancySmartCheckRateDivider)
			{
				if (SmartMidBucket.Num() > 0)
				{
					GetNextBucketBatch(LightBucketBatch, ELightBucket::SmartMid);
					ProcessRelevancyCheckLightBucketBatch(LightBucketBatch, ELightBucket::SmartMid);
				}
				MidSmartTimer = 0;
			}


			if (NearSmartTimer > 0.2 / RelevancySmartCheckRateDivider)
			{
				if (SmartNearBucket.Num() > 0)
				{
					GetNextBucketBatch(LightBucketBatch, ELightBucket::SmartNear);
					ProcessRelevancyCheckLightBucketBatch(LightBucketBatch, ELightBucket::SmartNear);
				}
				NearSmartTimer = 0;
			}

			break;
		}
	case ERelevancyCheckType::Octree:
		{
			UpdateOctreeLights();
			GetNextBucketBatch(LightBucketBatch, ELightBucket::Octree);
			ProcessRelevancyCheckLightBucketBatch(LightBucketBatch, ELightBucket::Octree);


			break;
		}

	default: ;
	}


	LastRelevancyUpdateLocation = GetOwner()->GetActorLocation();

	SET_DWORD_STAT(STAT_ALLLIGHTS, Entries.Num());
	SET_DWORD_STAT(STAT_SMARTNEAR, SmartNearBucket.Num());
	SET_DWORD_STAT(STAT_SMARTMID, SmartMidBucket.Num());
	SET_DWORD_STAT(STAT_SMARTFAR, SmartFarBucket.Num());
	SET_DWORD_STAT(STAT_RELEVANTLIGHTS, RelevantBucket.Num());
}


void ULXRDetectionComponent::ProcessRelevancyCheckLightBucketBatch(TArray<int32>& LightBatch, ELightBucket LightBucket)
{
	switch (RelevancyCheckType)
	{
	case ERelevancyCheckType::Fixed:
	case ERelevancyCheckType::Octree:

		for (int i = 0; i < LightBatch.Num(); ++i)
		{
			TWeakObjectPtr<ULXRSourceComponent> LightSourceComponent = Entries[LightBatch[i]].Light;
			int EntryIndex = LightBatch[i];
			if (LightSourceComponent.IsValid())
			{
				if (!RelevantBucket.Contains(EntryIndex))
				{
					bool bIsRelevant = LightSourceComponent->bAlwaysRelevant;
					if (!bIsRelevant)
					{
						bIsRelevant = CheckDistance(LightSourceComponent);
					}

					if (bIsRelevant)
					{
						AddRelevantLight(LightBatch[i]);
					}
				}
			}
		}
		break;

	case ERelevancyCheckType::Smart:
		{
			switch (LightBucket)
			{
			case ELightBucket::SmartFar:

				for (int i = 0; i < LightBatch.Num(); ++i)
				{
					TWeakObjectPtr<ULXRSourceComponent> LightSourceComponent = Entries[LightBatch[i]].Light;
					FLightEntry& Entry = Entries[LightBatch[i]];
					int EntryIndex = LightBatch[i];
					if (Entry.IsValid())
					{
#if UE_ENABLE_DEBUG_DRAWING
						if (DebugOptions.bDebugRelevancy)
						{
							constexpr float Radius = 30;
							constexpr float Thickness = 0;
							const FColor Color = FColor::Magenta;
							DrawDebugSphere(GetWorld(), LightSourceComponent->GetOwner()->GetActorLocation(), Radius, FMath::Clamp<int32>(Radius / 4.f, 8, 32), Color, false,
							                1 / RelevancySmartCheckRateDivider, 0,
							                Thickness);
						}
#endif


						const float DistSqr = FVector::DistSquared(LightSourceComponent->GetOwner()->GetActorLocation(), GetOwner()->GetActorLocation());
						if (DistSqr < (RelevancySmartDistanceMax * RelevancySmartDistanceMax * LightSourceComponent->AttenuationMultiplierToBeRelevant))
						{
							const ELightBucket NewType = GetSmartBucketForLightFromSqrDistance(DistSqr);
							MoveEntryToBucket(NewType, EntryIndex);
						}
					}
				}
				break;


			case ELightBucket::SmartMid:

				for (int i = 0; i < LightBatch.Num(); ++i)
				{
					TWeakObjectPtr<ULXRSourceComponent> LightSourceComponent = Entries[LightBatch[i]].Light;
					FLightEntry& Entry = Entries[LightBatch[i]];
					int EntryIndex = LightBatch[i];
					if (Entry.IsValid())
					{
#if UE_ENABLE_DEBUG_DRAWING
						if (DebugOptions.bDebugRelevancy)
						{
							const float Radius = 30;
							const float Thickness = 0;
							const FColor Color = FColor::Yellow;
							DrawDebugSphere(GetWorld(), LightSourceComponent->GetOwner()->GetActorLocation(), Radius, FMath::Clamp<int32>(Radius / 4.f, 8, 32), Color, false,
							                0.5 / RelevancySmartCheckRateDivider, 0,
							                Thickness);
						}
#endif
						const float DistSqr = FVector::DistSquared(LightSourceComponent->GetOwner()->GetActorLocation(), GetOwner()->GetActorLocation());
						const ELightBucket NewType = GetSmartBucketForLightFromSqrDistance(DistSqr);


						if (NewType != ELightBucket::SmartMid)
						{
							MoveEntryToBucket(NewType, EntryIndex);
							continue;
						}


						if (DistSqr < RelevancySmartDistanceMax * RelevancySmartDistanceMax * LightSourceComponent->AttenuationMultiplierToBeRelevant)
						{
							if (!RelevantBucket.Contains(EntryIndex))
							{
								bool bIsRelevant = LightSourceComponent->bAlwaysRelevant;

								if (!bIsRelevant)
								{
									RelevancyTaskData->Reset(GetTraceTargets(false));
									RelevancyTaskData->RelevancyRequiredAmountToPass = TargetsRequired < 1 ? RelevancyTaskData->ChosenTracePoints.Num() * TargetsRequired : TargetsRequired;

									RelevancyPassedData->SourceComponent = LightSourceComponent;
									bIsRelevant = ULXRFunctionLibrary::CheckIsLightRelevant(this, *RelevancyTaskData, *RelevancyPassedData, DebugOptions);
								}

								if (bIsRelevant)
								{
									AddRelevantLight(EntryIndex);
								}
							}
						}
					}
				}
				break;

			case ELightBucket::SmartNear:

				for (int i = 0; i < LightBatch.Num(); ++i)
				{
					TWeakObjectPtr<ULXRSourceComponent> LightSourceComponent = Entries[LightBatch[i]].Light;
					FLightEntry& Entry = Entries[LightBatch[i]];
					int EntryIndex = LightBatch[i];
					if (Entry.IsValid())
					{
#if UE_ENABLE_DEBUG_DRAWING
						if (DebugOptions.bDebugRelevancy)
						{
							const float Radius = 30;
							const float Thickness = 0;
							const FColor Color = FColor::Cyan;
							DrawDebugSphere(GetWorld(), LightSourceComponent->GetOwner()->GetActorLocation(), Radius, FMath::Clamp<int32>(Radius / 4.f, 8, 32), Color, false,
							                0.2 / RelevancySmartCheckRateDivider, 0,
							                Thickness);
						}
#endif
						const float DistSqr = FVector::DistSquared(LightSourceComponent->GetOwner()->GetActorLocation(), GetOwner()->GetActorLocation());
						const ELightBucket NewType = GetSmartBucketForLightFromSqrDistance(DistSqr);

						if (NewType != ELightBucket::SmartNear)
						{
							MoveEntryToBucket(NewType, EntryIndex);
							continue;
						}

						if (!RelevantBucket.Contains(EntryIndex))
						{
							bool bIsRelevant = LightSourceComponent->bAlwaysRelevant;
							if (!bIsRelevant)
							{
								RelevancyTaskData->Reset(GetTraceTargets(false));
								RelevancyPassedData->SourceComponent = LightSourceComponent;
								RelevancyTaskData->RelevancyRequiredAmountToPass = TargetsRequired < 1 ? RelevancyTaskData->ChosenTracePoints.Num() * TargetsRequired : TargetsRequired;
								bIsRelevant = ULXRFunctionLibrary::CheckIsLightRelevant(this, *RelevancyTaskData, *RelevancyPassedData, DebugOptions);
								//GEngine->AddOnScreenDebugMessage(uint64(this) + 97, GetWorld()->DeltaTimeSeconds, bIsRelevant ? FColor::Green : FColor::Red, FString::Printf(TEXT("SmartNearBucket")));
							}

							if (bIsRelevant)
							{
								AddRelevantLight(LightBatch[i]);
							}
						}
					}
				}
				break;
			default: ;
			}
		}
	default: ;
	}
}

void ULXRDetectionComponent::ProcessSmartEntry(int32 EntryIndex, ELightBucket CurrentBucket, float DebugLifetime, const FColor& DebugColor)
{
	FLightEntry* EntryPtr = nullptr;
	ULXRSourceComponent* Light = GetValidLight(EntryIndex, EntryPtr);
	if (!Light) return;

	DrawRelevancyDebug(Light, DebugLifetime, DebugColor);

	const float DistSqr = GetDistSqrToOwner(Light);

	// Bucket update rule differs for Far:
	// Far only migrates inward if inside MaxRange.
	if (CurrentBucket == ELightBucket::SmartFar)
	{
		if (IsInsideSmartMaxRangeSqr(DistSqr, Light))
		{
			const ELightBucket NewBucket = GetSmartBucketForLightFromSqrDistance(DistSqr);
			if (NewBucket != CurrentBucket)
			{
				MoveEntryToBucket(NewBucket, EntryIndex);
			}
		}
		return;
	}

	// Mid/Near: always recompute bucket; move if needed.
	const ELightBucket NewBucket = GetSmartBucketForLightFromSqrDistance(DistSqr);
	if (NewBucket != CurrentBucket)
	{
		MoveEntryToBucket(NewBucket, EntryIndex);
		return;
	}

	// Mid: only do relevancy work inside max range.
	if (CurrentBucket == ELightBucket::SmartMid && !IsInsideSmartMaxRangeSqr(DistSqr, Light))
	{
		return;
	}

	// Both Mid and Near: skip if already relevant
	if (RelevantBucket.Contains(EntryIndex))
	{
		return;
	}

	bool bIsRelevant = Light->bAlwaysRelevant;

	if (!bIsRelevant)
	{
		RelevancyTaskData->Reset(GetTraceTargets(false));
		RelevancyTaskData->RelevancyRequiredAmountToPass =
			TargetsRequired < 1.f
				? RelevancyTaskData->ChosenTracePoints.Num() * TargetsRequired
				: TargetsRequired;

		RelevancyPassedData->SourceComponent = Light;
		bIsRelevant = ULXRFunctionLibrary::CheckIsLightRelevant(this, *RelevancyTaskData, *RelevancyPassedData, DebugOptions);
	}

	if (!bIsRelevant)
	{
		return;
	}

	MoveEntryToBucket(ELightBucket::Relevant, EntryIndex);
}

FORCEINLINE ULXRSourceComponent* ULXRDetectionComponent::GetValidLight(int32 EntryIndex, FLightEntry*& OutEntry)
{
	if (!Entries.IsValidIndex(EntryIndex))
		return nullptr;

	FLightEntry& Entry = Entries[EntryIndex];
	OutEntry = &Entry;

	if (!Entry.IsValid())
		return nullptr;

	return Entry.Light.Get();
}

FORCEINLINE float ULXRDetectionComponent::GetDistSqrToOwner(const ULXRSourceComponent* Light) const
{
	return FVector::DistSquared(
		Light->GetOwner()->GetActorLocation(),
		GetOwner()->GetActorLocation()
	);
}

FORCEINLINE bool ULXRDetectionComponent::IsInsideSmartMaxRangeSqr(float DistSqr, const ULXRSourceComponent* Light) const
{
	const float MaxSqr = RelevancySmartDistanceMax * RelevancySmartDistanceMax * Light->AttenuationMultiplierToBeRelevant;
	return DistSqr < MaxSqr;
}

void ULXRDetectionComponent::DrawRelevancyDebug(const ULXRSourceComponent* Light, float Lifetime, const FColor& Color) const
{
#if UE_ENABLE_DEBUG_DRAWING
	if (!DebugOptions.bDebugRelevancy) return;

	constexpr float Radius = 30.f;
	DrawDebugSphere(
		GetWorld(),
		Light->GetOwner()->GetActorLocation(),
		Radius,
		FMath::Clamp<int32>(Radius / 4.f, 8, 32),
		Color,
		false,
		Lifetime,
		0,
		0.f
	);
#endif
}


void ULXRDetectionComponent::GetLXR()
{
	SCOPE_CYCLE_COUNTER(STAT_GetCombinedDatas);
	bool Reset = true;
	if (DetectionTaskData->PassedDatas.Num() > 0 || RelevantBucket.Num() > 0 || DetectionTaskData->TargetLXRData.IlluminatedTargetsCount > 0)
	{
		ULXRFunctionLibrary::GetLux(this, *DetectionTaskData, DetectionTaskData->PassedDatas, DetectionTaskData->TargetLXRData, DebugOptions);

		if (DetectionTaskData->TargetLXRData.IlluminatedTargetsCount > 0)
		{
			CombinedLXRColorTarget = FLinearColor(ULXRFunctionLibrary::GetLinearColorArrayAverage(DetectionTaskData->TargetLXRData.GetAllTargetColorsAsArray()));
			float MaxColor = ULXRFunctionLibrary::GetMaxOfColorChannels(CombinedLXRColor);

			float LuxToReturn = 0;
			switch (LuxReturnType)
			{
			case ELuxReturnType::AverageByTargets:
				LuxToReturn = GetWeightedAverage
					              ? DetectionTaskData->TargetLXRData.GetWeightedAverageLuxFromAllTargets(WeightedMinLuxThreshold, WeightToUseAboveMinLuxThreshold)
					              : DetectionTaskData->TargetLXRData.GetAverageLuxFromAllTargets();
				break;
			case ELuxReturnType::AverageByPassedTargets:
				LuxToReturn = GetWeightedAverage
					              ? DetectionTaskData->TargetLXRData.GetWeightedAverageLuxFromPassedTargets(WeightedMinLuxThreshold, WeightToUseAboveMinLuxThreshold)
					              : DetectionTaskData->TargetLXRData.GetAverageLuxFromPassedTargets();
				break;
			case ELuxReturnType::MaxOfPassedTargets:
				LuxToReturn = DetectionTaskData->TargetLXRData.GetMaxLux();
				break;
			case ELuxReturnType::WeightedSocketImportance:
				LuxToReturn = DetectionTaskData->TargetLXRData.GetSocketWeightedLux(*this);
				break;
			}
			CombinedLXRIntensityTarget = LuxToReturn;
			Reset = false;
		}
	}

	if (Reset)
	{
		CombinedLXRColorTarget = FLinearColor::Black;
		CombinedLXRIntensityTarget = 0;
	}

	float SmoothSpeed = CombinedLXRIntensityTarget < CombinedLXRIntensity ? 10000.f : LuxValueSmoothSpeed * 2;
	CombinedLXRColor = FLinearColor(FMath::VInterpConstantTo(FVector(CombinedLXRColor), FVector(CombinedLXRColorTarget), GetWorld()->DeltaTimeSeconds, SmoothSpeed));
	CombinedLXRIntensity = FMath::FInterpConstantTo(CombinedLXRIntensity, CombinedLXRIntensityTarget, GetWorld()->DeltaTimeSeconds, SmoothSpeed);

	if (bUserServerAuthority)
	{
		if (GetOwner()->HasAuthority())
		{
			if (FMath::Abs(CombinedLXRIntensity - Replicated_DetectedLightIntensity) >= IntensityUpdateTolerance)
			{
				Replicated_DetectedLightIntensity = CombinedLXRIntensity;
			}

			if (CombinedLXRColor.Equals(Replicated_DetectedLightColor, ColorUpdateTolerance))
			{
				Replicated_DetectedLightColor = CombinedLXRColor;
			}
		}
	}


#if UE_ENABLE_DEBUG_DRAWING
	if (DebugOptions.bDebugLXR)
	{
		ULXRFunctionLibrary::DebugOnGameThread([Weak = MakeWeakObjectPtr(this)]
		{
			if (Weak.IsValid())
			{
				auto SafeThis = Weak.Get();
				FVector loc = SafeThis->GetOwner()->GetActorLocation() + SafeThis->GetOwner()->GetActorRightVector() * -50 + (FVector::UpVector * 100);
				DrawDebugPoint(SafeThis->GetWorld(), loc, 15.f, SafeThis->CombinedLXRColor.ToFColor(true), false, 0.1f);
				DrawDebugPoint(SafeThis->GetWorld(), loc, 15.f, SafeThis->CombinedLXRColor.ToFColor(true), false, 0.1f);
			}
		});
	}
#endif
}

void ULXRDetectionComponent::Client_InterpolateCombinedIntensityTowardsReplicatedIntensity()
{
	if (!GetOwner()->HasAuthority())
	{
		CombinedLXRIntensity = FMath::FInterpTo(CombinedLXRIntensity, Replicated_DetectedLightIntensity, GetWorld()->DeltaTimeSeconds, 8.f);
	}
}

void ULXRDetectionComponent::Client_InterpolateCombinedColorTowardsReplicatedColor()
{
	if (!GetOwner()->HasAuthority())
	{
		CombinedLXRColor = FLinearColor(FMath::VInterpConstantTo(FVector(CombinedLXRColor), FVector(Replicated_DetectedLightColor), GetWorld()->DeltaTimeSeconds, 8.f));
	}
}


bool ULXRDetectionComponent::GetBucketByLightBucketType(ELightBucket LightBucketType, TArray<int32>& OutBucket)
{
	bool ReturnValue = false;
	switch (LightBucketType)
	{
	case ELightBucket::All:
		break;
	case ELightBucket::SmartNear:
		OutBucket = SmartNearBucket;
		ReturnValue = true;
		break;
	case ELightBucket::SmartMid:
		OutBucket = SmartMidBucket;
		ReturnValue = true;
		break;
	case ELightBucket::SmartFar:
		OutBucket = SmartFarBucket;
		ReturnValue = true;
		break;
	case ELightBucket::Octree:
		OutBucket = OctreeBucket;
		ReturnValue = true;
		break;
	case ELightBucket::Relevant:
		OutBucket = RelevantBucket;
		ReturnValue = true;
		break;
	case ELightBucket::Irrelevant:
		break;
	case ELightBucket::None:
		break;
	}

	return ReturnValue;
}


void ULXRDetectionComponent::GetNextBucketBatch(TArray<int32>& OutLightBatch, ELightBucket LightBucketType)
{
	const bool bIsRelevancyCheck = LightBucketType < ELightBucket::Relevant;
	const int& BatchCount = bIsRelevancyCheck ? RelevancyLightBatchCount : VisibilityLightBatchCount;
	TArray<int32> Bucket;
	if (GetBucketByLightBucketType(LightBucketType, Bucket))
	{
		int Index = GetCurrentBucketIndexByBucketType(LightBucketType);
		if (!Bucket.IsValidIndex((Index)))
			Index = Bucket.Num() - 1;

		if (Bucket.Num() == 0)
			Index = -1;

		const int MaxIteration = Bucket.Num() > BatchCount ? Bucket.Num() * 2 : Bucket.Num();
		int Iteration = 0;
		while (OutLightBatch.Num() != BatchCount && Iteration < MaxIteration)
		{
			if (Index == -1)
			{
				Index = Bucket.Num() - 1;
			}

			FLightEntry& Entry = Entries[Bucket[Index]];
			if (!Entry.IsValid())
			{
				Entry.Flags |= ELightFlags::PendingRemove;
				Pending.Add(Bucket[Index]);
				Iteration++;
				Index--;
				continue;
			}

			if (LXRSubsystem->bSoloFound)
			{
				if (Entry.Light->bSolo)
				{
					OutLightBatch.AddUnique(Bucket[Index]);
				}
			}
			else
			{
				OutLightBatch.AddUnique(Bucket[Index]);
			}

			Iteration++;
			Index--;
		}
		SetCurrentLightArrayIndexByLightBucketType(Index, LightBucketType);
	}
}


bool ULXRDetectionComponent::CheckDistance(TWeakObjectPtr<ULXRSourceComponent> LightSourceComponent) const
{
	if (!LightSourceComponent.IsValid()) return false;
	float Attenuation = 0;
	const FVector& Start = GetOwner()->GetActorLocation();
	const FVector& End = LightSourceComponent.Get()->GetOwner()->GetActorLocation();

	TArray<ULightComponent*> SourceLightComponents = LightSourceComponent.Get()->GetMyLightComponents();

	for (ULightComponent* ItLightComponent : SourceLightComponents)
	{
		if (ULocalLightComponent* LocalLightComponent = Cast<ULocalLightComponent>(ItLightComponent))
		{
			Attenuation = FMath::Max(LocalLightComponent->AttenuationRadius, Attenuation);
		}
	}
	const float DistanceToCheck = Attenuation * LightSourceComponent.Get()->AttenuationMultiplierToBeRelevant;
	const float DistanceSqr = FVector::DistSquared(Start, End);

	if (DistanceSqr < DistanceToCheck * DistanceToCheck)
		return true;
	return false;
}


void ULXRDetectionComponent::GetLightSystemLights()
{
	if (RelevancyCheckType == ERelevancyCheckType::Octree) return;

	for (TWeakObjectPtr<ULXRSourceComponent> Light : LXRSubsystem->GetAllLights())
	{
		if (LXRSubsystem->bSoloFound && !Light->bSolo) continue;
		if (Light->GetOwner() == GetOwner()) continue;

		int32 Index = Entries.Add({Light});
		Entries[Index].Flags |= ELightFlags::PendingAdd;
		LightToIndex.Add(Light.Get());
		LightToIndex[Light.Get()] = Index;
		Pending.Add(Index);
	}
}

int ULXRDetectionComponent::AllocateEntry(ULXRSourceComponent* Light)
{
	int32 Index;
	if (FreeSlots.Num() > 0)
	{
		Index = FreeSlots.Pop(EAllowShrinking::No);
	}
	else
	{
		Index = Entries.AddDefaulted();
	}

	FLightEntry& Entry = Entries[Index];
	Entry.Light = Light;
	Entry.bAlive = true;
	Entry.Bucket = ELightBucket::None;
	Entry.Flags = ELightFlags::PendingAdd;

	LightToIndex.Add(Entry.Light.Get(), Index);
	return Index;
}

void ULXRDetectionComponent::AddLight(ULXRSourceComponent* LightSource)
{
	if (LXRSubsystem)
	{
		int32 Index = Entries.Add({LightSource});
		LightToIndex.Add(LightSource);
		LightToIndex[LightSource] = Index;
	}
}

void ULXRDetectionComponent::RemoveLight(ULXRSourceComponent* LightSource)
{
	if (int32* FoundIndex = LightToIndex.Find(LightSource))
	{
		int32 EntryIndex = *FoundIndex;
		FLightEntry& Entry = Entries[EntryIndex];
		Entry.Flags |= ELightFlags::PendingRemove;
		Pending.Add(EntryIndex);
	}
}


void ULXRDetectionComponent::UpdateOctreeLights()
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR("Do fancy Octree work");

	for (auto& OctreeCheckerKVP : OctreeRelevancyChecker)
	{
		OctreeCheckerKVP.Value--;
		if (OctreeCheckerKVP.Value == 0)
		{
			FLightEntry& Entry = Entries[OctreeCheckerKVP.Key];
			Entry.Flags |= ELightFlags::PendingRemove;
			Pending.Add(OctreeCheckerKVP.Key);
		}
	}

	OctreeBoundsTestObject.Center = GetOwner()->GetActorLocation();
	OctreeBoundsTestObject.Extent = FVector4(FVector(RelevancyOctreeCheckBoundsSize));
	{
		LXRSubsystem->GetOctree()->FindElementsWithBoundsTest(OctreeBoundsTestObject, [this](const FLXROctreeElement& Element)
		{
			if (Element.GetOwner() == GetOwner() || Element.Data->OctreeElementType == Detection) return;
			if (Element.Data.Get().TreeActor.IsValid())
			{
				const TWeakObjectPtr<ULXRSourceComponent> SourceComponent = Element.Data.Get().SourceComponent;

				if (!LightToIndex.Contains(SourceComponent.Get()))
				{
					AllocateEntry(SourceComponent.Get());
				}
				if (int32* FoundIndex = LightToIndex.Find(SourceComponent.Get()))
				{
					int32 EntryIndex = *FoundIndex;
					MoveEntryToBucket(ELightBucket::Octree, EntryIndex);
					OctreeRelevancyChecker.Add(EntryIndex);
					OctreeRelevancyChecker[EntryIndex] = 5;
				}
			}
		});
	}

#if UE_ENABLE_DEBUG_DRAWING
	if (bDebugOctreeLights)
	{
		GEngine->AddOnScreenDebugMessage(uint64(this) + 52, 1.f, FColor::Magenta, FString::Printf(TEXT("Octree Lights: %d"), OctreeBucket.Num()));
		for (int i = 0; i < OctreeBucket.Num(); ++i)
		{
			FLightEntry& Entry = Entries[OctreeBucket[i]];
			if (Entry.IsValid())
			{
				DrawDebugDirectionalArrow(GetWorld(), GetOwner()->GetActorLocation(), Entry.Light->GetOwner()->GetActorLocation(), 150, FColor::Orange, false, RelevancyCheckRate, 0, 0);
			}
		}


		TArray<FLXROctreeSemantics::FLXROctree::FNodeIndex> NodeIndexes;
		TArray<FBoxCenterAndExtent> NodesBounds;
		int elementCount = 0;


		DebugOctreeCounter--;
		if (DebugOctreeCounter == 0)
		{
			LXRSubsystem->GetOctree()->FindNodesWithPredicate(
				[this, &NodesBounds](FLXROctreeSemantics::FLXROctree::FNodeIndex ParentNodeIndex, FLXROctreeSemantics::FLXROctree::FNodeIndex NodeIndex, const FBoxCenterAndExtent& NodeBounds)
				{
					NodesBounds.Add(NodeBounds);
					return true;
				}, [this,&elementCount, &NodeIndexes](FLXROctreeSemantics::FLXROctree::FNodeIndex ParentNodeIndex, FLXROctreeSemantics::FLXROctree::FNodeIndex NodeIndex,
				                                      const FBoxCenterAndExtent& NodeBounds)
				{
					NodeIndexes.Add(NodeIndex);
					FVector maxExtent = NodeBounds.Extent;
					FVector center = NodeBounds.Center;

					DrawDebugBox(GetWorld(), center, maxExtent, FColor::Blue, false, .5f);

					TArrayView<const FLXROctreeElement> elements = LXRSubsystem->GetOctree()->GetElementsForNode(NodeIndex);

					for (int i = 0; i < elements.Num(); i++)
					{
						// Draw debug boxes around elements
						float max = elements[i].Bounds.BoxExtent.GetMax();
						elementCount++;
					}
				});

			for (int i = 0; i < NodeIndexes.Num(); ++i)
			{
				TArrayView<const FLXROctreeElement> Elements = LXRSubsystem->GetOctree()->GetElementsForNode(NodeIndexes[i]);
				bool HasElements = Elements.Num() > 0;
				DrawDebugBox(GetWorld(), NodesBounds[i].Center, NodesBounds[i].Extent, HasElements ? FColor::Green : FColor::Red, false, 0.5f, 0, HasElements ? 10 : 5);
				for (auto Element : Elements)
				{
					if (!IsValid(Element.GetOwner())) continue;
					float Max = Element.Bounds.BoxExtent.GetMax();
					FVector MaxExtent = FVector(Max);
					FVector Center = Element.Data.Get().TreeActor->GetOwner()->GetActorLocation();

					DrawDebugBox(GetWorld(), Center, MaxExtent, FColor().Yellow, false, 0.5f);
				}
			}

			//GEngine->AddOnScreenDebugMessage(20940, 1.f, FColor::Magenta, FString::Printf(TEXT("Octree Elements: %d"), elementCount));
			DebugOctreeCounter = 10;
		}
	}
#endif

	bUpdateOctreeLights = false;
}

void ULXRDetectionComponent::SetParamsByQualityLevels()
{
	if (RelevancyCheckQuality < ECheckQuality::Custom)
	{
		RelevancyCheckRate = 0.1;
		RelevancySmartCheckRateDivider = 1;
		RelevancySmartDistanceMax = 2500;
		RelevancySmartDistanceMin = 1000;
		RelevancyOctreeCheckBoundsSize = 2000;
	}

	if (VisibilityCheckQuality < ECheckQuality::Custom)
	{
		VisibilityLightCheckRate = 0.05f;
	}

	switch (RelevancyCheckType)
	{
	case ERelevancyCheckType::Fixed:
		{
			switch (RelevancyCheckQuality)
			{
			case ECheckQuality::Low:
				RelevancyCheckRate *= 2;
				break;
			case ECheckQuality::Medium:
				RelevancyCheckRate *= 1;
				break;
			case ECheckQuality::High:
				RelevancyCheckRate *= 0.6;
				break;
			case ECheckQuality::Epic:
				RelevancyCheckRate *= 0.25;

				break;
			case ECheckQuality::Custom:
				break;
			default: ;
			}
		}
		break;
	case ERelevancyCheckType::Smart:
		{
			switch (RelevancyCheckQuality)
			{
			case ECheckQuality::Low:
				RelevancySmartCheckRateDivider *= 0.5;
				RelevancySmartDistanceMin *= .5;
				RelevancySmartDistanceMax *= .5;
				break;
			case ECheckQuality::Medium:
				RelevancySmartCheckRateDivider *= 1;
				RelevancySmartDistanceMin *= 1;
				RelevancySmartDistanceMax *= 1;

				break;
			case ECheckQuality::High:
				RelevancySmartCheckRateDivider *= 1.25;
				RelevancySmartDistanceMin *= 1;
				RelevancySmartDistanceMax *= 1.5;

				break;
			case ECheckQuality::Epic:
				RelevancySmartCheckRateDivider *= 1.55;
				RelevancySmartDistanceMin *= 1.5;
				RelevancySmartDistanceMax *= 2;

				break;
			case ECheckQuality::Custom:
				break;
			default: ;
			}
		}

		break;
	case ERelevancyCheckType::Octree:
		{
			switch (RelevancyCheckQuality)
			{
			case ECheckQuality::Low:
				break;
			case ECheckQuality::Medium:
				break;
			case ECheckQuality::High:
				break;
			case ECheckQuality::Epic:
				break;
			case ECheckQuality::Custom:
				break;
			default: ;
			}
		}

		break;
	default: ;
	}

	switch (VisibilityCheckQuality)
	{
	case ECheckQuality::Low:
		VisibilityLightCheckRate *= 4;
		break;
	case ECheckQuality::Medium:
		VisibilityLightCheckRate *= 1;
		break;
	case ECheckQuality::High:
		VisibilityLightCheckRate *= 0.75;
		break;
	case ECheckQuality::Epic:
		VisibilityLightCheckRate *= 0.25;
		break;
	case ECheckQuality::Custom:
		break;
	default: ;
	}
}


void ULXRDetectionComponent::RemoveStaleLights()
{
	TArray<int> StaleLightsIndexes;

	for (int i = 0; i < Entries.Num(); ++i)
	{
		TWeakObjectPtr<ULXRSourceComponent> LightSourceComponent = Entries[i].Light;
		if (!LightSourceComponent.IsValid())
		{
			if (LightSourceComponent.IsStale())
			{
				StaleLightsIndexes.AddUnique(i);
			}
		}
	}

	if (StaleLightsIndexes.Num() > 0)
	{
		for (int i = 0; i < StaleLightsIndexes.Num(); ++i)
		{
			if (Entries.IsValidIndex(StaleLightsIndexes[i]))
			{
				FLightEntry& Entry = Entries[StaleLightsIndexes[i]];
				Entry.Flags |= ELightFlags::PendingRemove;
				Pending.Add(StaleLightsIndexes[i]);
			}
		}
	}
}

TMap<int, FLXR> ULXRDetectionComponent::GetIlluminatedTargets()
{
	return DetectionTaskData->TargetLXRData.TargetsLXR;
}

TArray<FVector> ULXRDetectionComponent::GetVisibilityTraceTypeTargets() const
{
	return GetTraceTargets(true);
}

TArray<ULXRSourceComponent*> ULXRDetectionComponent::GetPassedLights() const
{
	TArray<ULXRSourceComponent*> ReturnList;
	for (auto It = DetectionTaskData->PassedDatas.CreateConstIterator(); It; ++It)
	{
		if (It->GetAllPassedTargets().Num() > 0)
			ReturnList.Add(It->SourceComponent.Get());
	}

	return ReturnList;
}

TArray<ULightComponent*> ULXRDetectionComponent::GetPassedLightComponents(ULXRSourceComponent* InSourceComponent)
{
	if (!IsValid(InSourceComponent)) return {};
	TArray<ULightComponent*> ReturnList;
	TArray<ULightComponent*> LightComponents = InSourceComponent->GetMyLightComponents();

	FLightSourcePassedData* Data = nullptr;
	for (auto& Element : DetectionTaskData->PassedDatas)
	{
		if (Element.SourceComponent == InSourceComponent)
		{
			Data = &Element;
			break;
		}
	}
	if (Data != nullptr)
	{
		for (auto ComponentIndex : Data->GetAllPassedComponentsAsArray())
		{
			ReturnList.Add(LightComponents[ComponentIndex]);
		};
	}


	return ReturnList;
}


FName ULXRDetectionComponent::GetSocketNameByTraceTargetIndex(int TraceTargetIndex)
{
	return SocketNameToTraceTarget[TraceTargetIndex];
}

int ULXRDetectionComponent::GetTraceTargetIndexBySocketName(FName SocketName) const
{
	return SocketNameToTraceTarget.IndexOfByKey(SocketName);
}

TArray<FVector> ULXRDetectionComponent::GetTraceTargets(const bool& bIsRelevant, const ETraceTarget TargetOverride) const
{
	TArray<FVector> Temp;

	const ETraceTarget TargetType = TargetOverride != ETraceTarget::None ? TargetOverride : bIsRelevant ? VisibilityTargetType : RelevancyTargetType;
	switch (TargetType)
	{
	case ETraceTarget::ActorLocation:
		{
			Temp.AddUnique(GetOwner()->GetActorLocation());
			break;
		}
	case ETraceTarget::Sockets:
		{
			for (FName Socket : CachedSockets)
			{
				if (RealSkeletalMeshComponent)
				{
					Temp.AddUnique(RealSkeletalMeshComponent->GetSocketLocation(Socket));
				}
			}
		}
		break;

	case ETraceTarget::VectorArray:
		{
			for (auto Vector : TargetVectors)
			{
				Temp.Add(GetOwner()->GetTransform().TransformPosition(Vector));
			}
		}
		break;
	case ETraceTarget::ActorBounds:
		{
			FVector Origin;
			FVector Extent;
			GetOwner()->GetActorBounds(true, Origin, Extent);
			Extent = Extent / 1.5;
			Temp.AddUnique(GetOwner()->GetTransform().TransformPosition(FVector(0, 0, Extent.Z)));
			Temp.AddUnique(GetOwner()->GetTransform().TransformPosition(FVector(0, 0, -Extent.Z + 15)));
			Temp.AddUnique(GetOwner()->GetTransform().TransformPosition(FVector(0, Extent.Y * 0.5, Extent.Z * 0.1)));
			Temp.AddUnique(GetOwner()->GetTransform().TransformPosition(FVector(0, -Extent.Y * 0.5, Extent.Z * 0.1)));
			Temp.AddUnique(GetOwner()->GetTransform().TransformPosition(FVector(0, -Extent.Y * 0.5, Extent.Z * 0.1)));
			Temp.AddUnique(GetOwner()->GetTransform().TransformPosition(FVector(Extent.X * 0.5, 0, 0)));
			Temp.AddUnique(GetOwner()->GetTransform().TransformPosition(FVector(-Extent.X * 0.5, 0, 0)));
		}
		break;
	case ETraceTarget::None:
		break;
	default:
		break;
	}

#if UE_ENABLE_DEBUG_DRAWING
	if (DebugOptions.bDebugRelevancy)
		for (auto& Element : Temp)
		{
			DrawDebugCrosshairs(GetWorld(), Element, FRotator::MakeFromEuler(FVector::UpVector), 20.f, bIsRelevant ? FColor::Emerald : FColor::Cyan, false, 0.25f);
		}
#endif


	return Temp;
}

void ULXRDetectionComponent::DisplayDebug(AHUD* HUD, UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& YL, float& YPos)
{
	auto TaskStateToString([](ETaskState State) -> FString
	{
		switch (State)
		{
		case ETaskState::Cleanup:
			return "Cleanup";
		case ETaskState::GeneratingTargets:
			return "GeneratingTargets";
		case ETaskState::Processing:
			return "Processing";
		case ETaskState::Analyze:
			return "Analyze";
		case ETaskState::Max:
			return "Max";
		}
		return "";
	});
	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;
	DisplayDebugManager.SetDrawColor(FColor(255, 255, 0));
	{
		DisplayDebugManager.DrawString(FString(TEXT("------------------------------")));
		DisplayDebugManager.DrawString(FString::Printf(TEXT("%s Detection Component"), *GetName()));
		DisplayDebugManager.DrawString(FString(TEXT("------------------------------")));
		DisplayDebugManager.DrawString(FString::Printf(TEXT("TaskState : %s"), *TaskStateToString(DetectionTaskData->GetTaskState())));
		DisplayDebugManager.DrawString(FString::Printf(TEXT("AllLights %d"), Entries.Num()));
		DisplayDebugManager.DrawString(FString::Printf(TEXT("SmartNear %d"), SmartNearBucket.Num()));
		DisplayDebugManager.DrawString(FString::Printf(TEXT("SmartMid %d"), SmartMidBucket.Num()));
		DisplayDebugManager.DrawString(FString::Printf(TEXT("SmartFar %d"), SmartFarBucket.Num()));
		DisplayDebugManager.DrawString(FString::Printf(TEXT("RelevantLights %d"), RelevantBucket.Num()));
		DisplayDebugManager.DrawString(FString::Printf(TEXT("Pending %d"), Pending.Num()));

		DisplayDebugManager.DrawString(FString(TEXT("------------------------------")));
	}
}

// Vibe Coded debugs
FString ULXRDetectionComponent::FormatBytes(int64 Bytes)
{
	const double KB = Bytes / 1024.0;
	const double MB = KB / 1024.0;

	if (MB >= 1.0) return FString::Printf(TEXT("%.2f MB"), MB);
	if (KB >= 1.0) return FString::Printf(TEXT("%.2f KB"), KB);
	return FString::Printf(TEXT("%lld B"), Bytes);
}

void ULXRDetectionComponent::AppendArrayStat(FString& Out, const TCHAR* Name, const TArray<int32>& Arr)
{
	const int64 Alloc = Arr.GetAllocatedSize();
	Out += FString::Printf(
		TEXT("  %-18s  Num=%6d  Max=%6d  Alloc=%10s (%lld B)\n"),
		Name, Arr.Num(), Arr.Max(), *FormatBytes(Alloc), Alloc
	);
}

void ULXRDetectionComponent::AppendEntryArrayStat(FString& Out, const TCHAR* Name, const TArray<FLightEntry>& Arr)
{
	const int64 Alloc = Arr.GetAllocatedSize();
	Out += FString::Printf(
		TEXT("  %-18s  Num=%6d  Max=%6d  ElemSize=%3d  Alloc=%10s (%lld B)\n"),
		Name, Arr.Num(), Arr.Max(), int32(sizeof(FLightEntry)), *FormatBytes(Alloc), Alloc
	);
}

void ULXRDetectionComponent::AppendMapStat(FString& Out, const TCHAR* Name, const TMap<TWeakObjectPtr<ULXRSourceComponent>, int32>& Map)
{
	const int64 Alloc = Map.GetAllocatedSize();


	Out += FString::Printf(
		TEXT("  %-18s  Num=%6d  Alloc=%10s (%lld B)\n"),
		Name, Map.Num(), *FormatBytes(Alloc), Alloc
	);
}

void ULXRDetectionComponent::DebugPrintLightRegistryMemory() const
{
	FString S;
	S += TEXT("=== LXR Light Registry Memory ===\n");

	int64 Total = 0;

	// Entries
	{
		const int64 Alloc = Entries.GetAllocatedSize();
		Total += Alloc;
		AppendEntryArrayStat(S, TEXT("Entries"), Entries);
	}

	// Map
	{
		const int64 Alloc = LightToIndex.GetAllocatedSize();
		Total += Alloc;
		AppendMapStat(S, TEXT("LightToIndex"), LightToIndex);
	}

	// Pending
	{
		const int64 Alloc = Pending.GetAllocatedSize();
		Total += Alloc;
		AppendArrayStat(S, TEXT("Pending"), Pending);
	}

	// Buckets
	for (int32 i = 0; i < UE_ARRAY_COUNT(Buckets); ++i)
	{
		const int64 Alloc = Buckets[i].GetAllocatedSize();
		Total += Alloc;

		const TCHAR* BucketName = TEXT("Bucket");
		switch (static_cast<ELightBucket>(i))
		{
		case ELightBucket::SmartNear: BucketName = TEXT("Bucket:SmartNear");
			break;
		case ELightBucket::SmartMid: BucketName = TEXT("Bucket:SmartMid");
			break;
		case ELightBucket::SmartFar: BucketName = TEXT("Bucket:SmartFar");
			break;
		case ELightBucket::Octree: BucketName = TEXT("Bucket:Octree");
			break;
		case ELightBucket::Relevant: BucketName = TEXT("Bucket:Relevant");
			break;
		default: BucketName = TEXT("Bucket:Other");
			break;
		}

		AppendArrayStat(S, BucketName, Buckets[i]);
	}

	S += FString::Printf(TEXT("--------------------------------\n  TOTAL Alloc=%s (%lld B)\n"), *FormatBytes(Total), Total);

	UE_LOG(LogTemp, Log, TEXT("%s"), *S);
}
