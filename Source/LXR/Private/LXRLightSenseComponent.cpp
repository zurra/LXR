// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#include "LXRLightSenseComponent.h"
#include "DrawDebugHelpers.h"
#include "ILXRSense.h"
#include "LXRFunctionLibrary.h"
#include "LXRMemoryComponent.h"
#include "LXRMethodObject.h"
#include "LXRSilhouetteDetectionComponent.h"
#include "LXRSourceComponent.h"
#include "LXRSubsystem.h"
#include "LXR_Shared.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponent.h"
#include "Components/LocalLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Tasks/GenerateSenseTargetsAsyncTask.h"
#include "Tasks/ProcessSenseTaskAsyncTask.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"

#if WITH_EDITOR
void ULXRLightSenseComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, SenseCheckQuality))
	{
		SenseTraceTaskParams = FConeTraceTaskParams(SenseCheckQuality);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

ULXRLightSenseComponent::ULXRLightSenseComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	// PrimaryComponentTick.RegisterTickFunction(GetComponentLevel());
	SetComponentTickInterval(CheckRate);
}

void ULXRLightSenseComponent::BeginPlay()
{
	Super::BeginPlay();
	Params = FCollisionQueryParams(GetOwner()->GetFName(), SCENE_QUERY_STAT_ONLY(KismetTraceUtils), false);
	Params.AddIgnoredActor(GetOwner());

	SenseTaskData = MakeShared<FSenseTaskData>();

	if (ULXRMemoryComponent* MemoryComponent = GetOwner()->FindComponentByClass<ULXRMemoryComponent>())
	{
		IsLightMemoryEnabled = MemoryComponent->MemoryCheckClass == EMemoryCheckClass::Sense;

		SenseTaskData->LightMemoryEnabled = IsLightMemoryEnabled;
		SenseTaskData->MemoryComponent = MemoryComponent;
	}


	SenseTaskData->SenseComponent = this;
	SenseTaskData->TraceType = EDetectionType::LightSense;


	if (SenseTraceTransform == ETraceTransform::MethodObject)
	{
		if (LXRMethodObject.IsValid())
		{
			const UClass* MethodObjectClass = LXRMethodObject.TryLoadClass<ULXRMethodObject>();

			MethodObject = NewObject<ULXRMethodObject>(GetOwner(), MethodObjectClass);
			MethodObject->SetOwner(GetOwner());
			MethodObject->SetLightSenseComponent(this);
		}
	}
}

void ULXRLightSenseComponent::TickComponent(float DeltaTime, ELevelTick Tick, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, Tick, ThisTickFunction);
	if (GetComponentTickInterval() != CheckRate)
		SetComponentTickInterval(CheckRate);
	switch (SenseTaskData->GetTaskState())
	{
	case ETaskState::Cleanup:
		if (SenseTaskData->IsStateStarted()) break;
		SenseTaskData->StartState();
		CheckAndRemoveNotValidOrStaleLights();
		if (SenseTaskData->SensedLightsToRemove.Num() > 0)
		{
			if (!SenseTaskData->TaskLock)
			{
				for (TWeakObjectPtr<ULXRSourceComponent> LightToRemove : SenseTaskData->SensedLightsToRemove)
				{
					RemoveSensedLight(LightToRemove);
				}
				SenseTaskData->SensedLightsToRemove.Reset();
			}
		}
		SenseTaskData->NextState(); //generate targets
		break;

	case ETaskState::GeneratingTargets:
		if (!SenseTaskData->IsStateStarted())
			GenerateSenseTargets();
		break;

	case ETaskState::Processing:
		if (!SenseTaskData->IsStateStarted())
			ProcessSenseTask();

		break;
	case ETaskState::Analyze:
		if (!SenseTaskData->IsStateStarted())
		{
			GetSenseLXR();
			SenseTaskData->NextState(); //cleanup
		}
		break;
	case ETaskState::Max:
		break;
	}


	CombinedLXRColor = FLinearColor(FMath::VInterpConstantTo(FVector(CombinedLXRColor), FVector(LastCombinedLXRColor), GetWorld()->DeltaTimeSeconds, 25.f));
	CombinedLXRIntensity = FMath::FInterpConstantTo(CombinedLXRIntensity, LastCombinedLXRIntensity, GetWorld()->DeltaTimeSeconds, 25.f);
}

void ULXRLightSenseComponent::GetSenseLXR()
{
	SCOPE_CYCLE_COUNTER(STAT_GetCombinedSenseDatas);

	SenseTaskData->StartState();
	if (SenseTaskData->SensedLightPassedData.Num() > 0)
	{
		ULXRFunctionLibrary::GetLux(this, *SenseTaskData, SenseTaskData->PassedDatas, SenseTaskData->TargetLXRData, {});

		if (SenseTaskData->TargetLXRData.IlluminatedTargetsCount == 0)
		{
			CheckPassedLightsForRelevancy = true;
		}
		else
		{
			if (!CheckPassedLightsForRelevancy)
			{
				CheckPassedLightsForRelevancy = true;
				CurrentCombinedLXRColor = FLinearColor(ULXRFunctionLibrary::GetLinearColorArrayAverage(SenseTaskData->TargetLXRData.GetAllTargetColorsAsArray()));
				CurrentCombinedLXRIntensity = SenseTaskData->TargetLXRData.GetAverageLuxFromPassedTargets();

				CurrentCombinedLXRIntensity = FMath::Min(CurrentCombinedLXRColor.GetMax(), CurrentCombinedLXRIntensity);
			}
		}
	}
	else
	{
		// GEngine->AddOnScreenDebugMessage(80, 0.1f, FColor::Magenta, FString::Printf(TEXT("PassedTargetCounter ==== 0 && Sensedlight 0")));
		CurrentCombinedLXRColor = FLinearColor(FMath::VInterpConstantTo(FVector(LastCombinedLXRColor), FVector::ZeroVector, GetWorld()->DeltaTimeSeconds, 25.f));
		CurrentCombinedLXRIntensity = FMath::FInterpConstantTo(LastCombinedLXRIntensity, 0, GetWorld()->DeltaTimeSeconds, 50.f);
	}


#if UE_ENABLE_DEBUG_DRAWING
	if (DebugOptions.bDebugLXR)
	{
		ULXRFunctionLibrary::DebugOnGameThread([Weak = MakeWeakObjectPtr(this)]
		{
			if (Weak.IsValid())
			{
				auto SafeThis = Weak.Get();
				FVector loc = SafeThis->GetOwner()->GetActorLocation() + SafeThis->GetOwner()->GetActorRightVector() * -50 + (FVector::UpVector * 150);
				DrawDebugPoint(SafeThis->GetWorld(), loc, 15.f, SafeThis->CombinedLXRColor.ToFColor(true), false, 0.2f);
			}
		});
	}
#endif

	LastCombinedLXRColor = CurrentCombinedLXRColor;
	LastCombinedLXRIntensity = CurrentCombinedLXRIntensity;
}

void ULXRLightSenseComponent::ProcessSenseTask()
{
	if (SenseTaskData->IsStateStarted()) return;

	if (SenseTaskData->TargetIterator == -1 || SenseTaskData->TracePoints.Num() == 0)
	{
		SenseTaskFinished();
		return;
	}

	SenseTaskData->DecreaseSourcesRefTimer();

	switch (SenseCalculationType)
	{
	case ETaskProcessType::Sync:
		SenseTaskData->IsBackgroundThread = false;
		(new FAutoDeleteAsyncTask<FProcessSenseTaskAsyncTask>(SenseTaskData))->StartSynchronousTask();
		break;

	case ETaskProcessType::Multithread:
		SenseTaskData->IsBackgroundThread = true;
		(new FAutoDeleteAsyncTask<FProcessSenseTaskAsyncTask>(SenseTaskData))->StartBackgroundTask();
		break;

	default: ;
	}
}

void ULXRLightSenseComponent::SenseTaskFinished()
{
	for (FLightSourcePassedData& Element : SenseTaskData->SensedLightPassedData)
	{
		if (!SenseTaskData->IsSourceSensedThisTick(Element.SourceComponent))
		{
			SenseTaskData->SensedLightsToRemove.Add(Element.SourceComponent);
		}
	}
	SenseTaskData->TaskLock = false;
	SenseTaskData->NextState(); // Analyzing
}


TArray<AActor*> ULXRLightSenseComponent::GetSensedActors() const
{
	TArray<AActor*> SensedActors;

	for (auto& Data : SenseTaskData->SensedLightPassedData)
	{
		if (Data.SourceComponent.IsValid() && !Data.SourceComponent.IsStale())
		{
			SensedActors.Add(Data.SourceComponent->GetOwner());
		}
	}
	return SensedActors;
}

void ULXRLightSenseComponent::AddSensedLightFromThread(const FLightSourcePassedData& PassedData)
{
	FRWScopeLock RelevantLightLock(RelevantDataLockObject, SLT_Write);
	AddSensedLight(PassedData);
}


void ULXRLightSenseComponent::AddSensedLight(const FLightSourcePassedData& PassedData)
{
	if (SenseTaskData->TaskLock) return;

	SenseTaskData->TaskLock = true;
	if (PassedData.SourceComponent.IsValid() && !PassedData.SourceComponent.IsStale())
	{
		FLightSourcePassedData* Data = nullptr;
		for (FLightSourcePassedData& Element : SenseTaskData->SensedLightPassedData)
		{
			if (Element.SourceComponent == PassedData.SourceComponent)
			{
				Data = &Element;
				break;
			}
		}
		if (Data == nullptr)
		{
			auto ElementID = SenseTaskData->SensedLightPassedData.Add(PassedData);
			Data = &SenseTaskData->SensedLightPassedData[ElementID];
		}

		SenseTaskData->SetSourceAsSensed(Data);

		Data->PassedComponentsAndTargets = PassedData.PassedComponentsAndTargets;
	}
	SenseTaskData->TaskLock = false;
}

void ULXRLightSenseComponent::RemoveSensedLightFromThread(const TWeakObjectPtr<ULXRSourceComponent>& SensedLight)
{
	FRWScopeLock RelevantLightLock(RelevantDataLockObject, SLT_Write);
	RemoveSensedLight(SensedLight);
}

void ULXRLightSenseComponent::RemoveSensedLight(const TWeakObjectPtr<ULXRSourceComponent>& SensedLight)
{
	if (SenseTaskData->TaskLock)
		SenseTaskData->SensedLightsToRemove.Add(SensedLight);
	else
	{
		SenseTaskData->TaskLock = true;
		const FLightSourcePassedData* ElementToRemove = nullptr;
		for (FLightSourcePassedData& Element : SenseTaskData->SensedLightPassedData)
		{
			if (Element.SourceComponent == SensedLight)
			{
				ElementToRemove = &Element;
				break;
			}
		}
		if (ElementToRemove != nullptr)
		{
			SenseTaskData->SensedLightPassedData.Remove(*ElementToRemove);
		}
	}
	SenseTaskData->SourcesSensedThisTick.Remove(SensedLight);
	SenseTaskData->TaskLock = false;
}

void ULXRLightSenseComponent::ResetLXRSenseArrays()
{
	if (SenseTaskData->TaskLock)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ULXRLightSenseComponent::ResetLXRSenseArrays);
		return;
	}
	// GEngine->AddOnScreenDebugMessage(static_cast<int32>(GetUniqueID()), 0.5f, FColor::Magenta, FString::Printf(TEXT("LightSense REMOVE LIGHTS")));
	SenseTaskData->TaskLock = true;
	SenseTaskData->SensedLightPassedData.Reset();
	SenseTaskData->TaskLock = false;
}

void ULXRLightSenseComponent::ProcessGeneratedTargets()
{
	TArray<int> IndexesToRemove;

	// AsyncTask(ENamedThreads::GameThread, [&]()
	// {
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypeQueries;
	ObjectTypeQueries.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
	for (int i = NewTraceTargets.Num() - 1; i >= 0; --i)
	{
		TArray<AActor*> OverlappingActors;
		UKismetSystemLibrary::SphereOverlapActors(this, NewTraceTargets[i], 1, ObjectTypeQueries,NULL, {}, OverlappingActors);

		if (OverlappingActors.Num() > 0)
			NewTraceTargets.RemoveAt(i);
	}
	// });


	if (NewTraceTargets.Num() > 0)
	{
		const int32 LastIndex = NewTraceTargets.Num() - 1;
		for (int32 i = 0; i <= LastIndex; ++i)
		{
			const int32 Index = FMath::RandRange(i, LastIndex);
			if (i != Index)
			{
				NewTraceTargets.Swap(i, Index);
			}
		}
	}

	SenseTaskData->SetNewTracePointsAndResetIterator(NewTraceTargets);
	IsBackgroundThreadRunning = false;
	GotTargetsFromThread = false;

	if (DebugOptions.bDebugTargets)
	{
		for (FVector& TraceTarget : SenseTaskData->TracePoints)
		{
			DrawDebugBox(GetWorld(), TraceTarget, FVector(5), FColor::Emerald, false, CheckRate);
		}
	}
}

void ULXRLightSenseComponent::GenerateSenseTargets()
{
	// FVector Location;
	// FRotator Rotation;
	GetSenseLocationAndRotation(SenseTaskData->SenseLocation, SenseTaskData->SenseRotation);
	// // GenerateSenseTargets(Location, Rotation);

	RefreshOctreeLights = true;
	SenseTaskData->Lights = GetOctreeLights();


	switch (SenseCalculationType)
	{
	case ETaskProcessType::Sync:
		{
			SenseTaskData->IsBackgroundThread = false;
			(new FAutoDeleteAsyncTask<FGenerateSenseTargetsAsyncTask>(SenseTaskData))->StartSynchronousTask();
		}

		break;

	case ETaskProcessType::Multithread:
		{
			SenseTaskData->IsBackgroundThread = true;
			(new FAutoDeleteAsyncTask<FGenerateSenseTargetsAsyncTask>(SenseTaskData))->StartBackgroundTask();
		}

		break;

	default: ;
	}
}

void ULXRLightSenseComponent::GetSenseLocationAndRotation(FVector& OutLocation, FRotator& OutRotation) const
{
	switch (SenseTraceTransform)
	{
	case ETraceTransform::None:
		break;
	case ETraceTransform::Actor:
		{
			OutLocation = GetOwner()->GetActorLocation();
			OutRotation = GetOwner()->GetActorForwardVector().Rotation();
		}
		break;
	case ETraceTransform::ActorEyesViewPoint:
		{
			GetOwner()->GetActorEyesViewPoint(OutLocation, OutRotation);
		}
		break;
	case ETraceTransform::Custom:
		{
			if (GetOwner()->GetClass()->ImplementsInterface(ULXRSense::StaticClass()))
			{
				ILXRSense::Execute_GetLightSenseTraceLocationAndDirection(GetOwner(), OutLocation, OutRotation);
			}
		}
		break;
	case ETraceTransform::Socket:
		{
			if (const USkeletalMeshComponent* SkeletalMeshComponent = GetOwner()->FindComponentByClass<USkeletalMeshComponent>())
			{
				if (SkeletalMeshComponent->DoesSocketExist(SocketName))
				{
					SkeletalMeshComponent->GetSocketWorldLocationAndRotation(SocketName, OutLocation, OutRotation);
				}
			}
		}
		break;
	default: ;
	}
}

void ULXRLightSenseComponent::GetSenseLocationAndRotationForSilhouetteSense(AActor* SilhouetteActor, FVector& OutLocation, FRotator& OutRotator) const
{
	OutRotator = UKismetMathLibrary::FindLookAtRotation(GetOwner()->GetActorLocation(), SilhouetteActor->GetActorLocation());
	OutLocation = SilhouetteActor->GetActorLocation() * OutRotator.Vector() * 100;
}


TArray<TWeakObjectPtr<ULXRSourceComponent>> ULXRLightSenseComponent::GetOctreeLights()
{
	if (RefreshOctreeLights)
	{
		OctreeLights.Reset();
		FVector Location;
		FRotator Rotation;
		GetSenseLocationAndRotation(Location, Rotation);


		const FVector RelativeLocation = GetOwner()->GetTransform().InverseTransformPosition(Location);
		const FVector LocalSpaceLocationForOctreeQuery = RelativeLocation + GetOwner()->GetActorRotation().UnrotateVector(Rotation.Vector()) * SenseTraceTaskParams.SenseDistance / 2;

		OctreeBoundsTestObject.Center = GetOwner()->GetActorTransform().TransformPosition(LocalSpaceLocationForOctreeQuery);
		OctreeBoundsTestObject.Extent = FVector4(FVector(SenseTraceTaskParams.SenseDistance));
		const ULXRSubsystem* LXRSubsystem = GetOwner()->GetWorld()->GetSubsystem<ULXRSubsystem>();
		bool bSoloFound = LXRSubsystem->bSoloFound;

		LXRSubsystem->GetOctree()->FindElementsWithBoundsTest(OctreeBoundsTestObject, [this,bSoloFound](const FLXROctreeElement& Element)
		{
			if (Element.GetOwner() == GetOwner() || Element.Data->OctreeElementType == Detection) return;

			if (Element.Data.Get().TreeActor.IsValid())
			{
				const TWeakObjectPtr<ULXRSourceComponent> SourceComponent = Element.Data.Get().SourceComponent;
				const bool AddForMemoryPurposes = IsLightMemoryEnabled && SourceComponent->bIsMemorizable;
				if (bSoloFound && !SourceComponent->bSolo) return;

				if (SourceComponent->bEnableLightSense || AddForMemoryPurposes)
					OctreeLights.AddUnique(SourceComponent);
			}
		});
	}


	if (OctreeLights.Num() == 0)
	{
		RefreshOctreeLights = true;
		ResetLXRSenseArrays();
	}
	if (CheckPassedLightsForRelevancy)
	{
		if (SenseTaskData->SensedLightPassedData.Num() > 0)
			for (FLightSourcePassedData& Data : SenseTaskData->SensedLightPassedData)
			{
				if (!OctreeLights.Contains(Data.SourceComponent))
					OctreeLights.Add(Data.SourceComponent);
			}
		CheckPassedLightsForRelevancy = false;
	}

	return OctreeLights;
}

void ULXRLightSenseComponent::CheckAndRemoveNotValidOrStaleLights()
{
	if (SenseTaskData->TaskLock) return;

	SenseTaskData->TaskLock = true;

	TArray<FLightSourcePassedData> ToRemove;

	for (auto& Data : SenseTaskData->SensedLightPassedData)
	{
		if (!Data.SourceComponent.IsStale())
		{
			if (Data.SourceComponent.Get())
			{
				continue;
			}
		}
		ToRemove.Add(Data);
	}
	for (auto& Data : ToRemove)
	{
		SenseTaskData->SensedLightPassedData.Remove(Data);
	}
	SenseTaskData->TaskLock = false;
}
