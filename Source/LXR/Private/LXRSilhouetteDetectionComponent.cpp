// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.


#include "LXRSilhouetteDetectionComponent.h"
#include "ILXRSilhouette.h"
#include "LXRFunctionLibrary.h"
#include "LXRMethodObject.h"
#include "LXRSourceComponent.h"
#include "LXRSubsystem.h"
#include "LXR_Shared.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "Tasks/GenerateSilhouetteTargetsAsyncTask.h"
#include "Tasks/ProcessSilhouetteTaskAsyncTask.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"

#if WITH_EDITOR
void ULXRSilhouetteDetectionComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, SilhouetteCheckQuality))
	{
		SilhouetteTraceTaskParams = FSilhouetteTraceTaskParams(SilhouetteCheckQuality);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

// Sets default values for this component's properties
ULXRSilhouetteDetectionComponent::ULXRSilhouetteDetectionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	UActorComponent::SetComponentTickEnabled(false);
	SetComponentTickInterval(0.1f);
	// ...
}


// Called when the game starts
void ULXRSilhouetteDetectionComponent::BeginPlay()
{
	Super::BeginPlay();
	// if (!bDrawDebug) return;

	SetComponentTickEnabled(false);
	LXRSubsystem = GetOwner()->GetWorld()->GetSubsystem<ULXRSubsystem>();

	//Lazy "late begin play"
	FTimerHandle Temp;
	LXRSubsystem = GetOwner()->GetWorld()->GetSubsystem<ULXRSubsystem>();
	GetWorld()->GetTimerManager().SetTimer(Temp, FTimerDelegate::CreateLambda([&]
	{
		FillNearbyDetectionComponents();
		PrimaryComponentTick.SetTickFunctionEnable(true);
	}), 0.5f, false);

	if (TraceTransform == ETraceTransform::MethodObject)
	{
		if (LXRMethodObject.IsValid())
		{
			const UClass* MethodObjectClass = LXRMethodObject.TryLoadClass<ULXRMethodObject>();

			// todo this some time....
			// ULXRSubsystem* LXRSubSystem = GetOwner()->GetWorld()->GetSubsystem<ULXRSubsystem>();
			// MethodObject = LXRSubSystem->GetOrAddMethodObject(MethodObjectClass);

			MethodObject = NewObject<ULXRMethodObject>(GetOwner(), MethodObjectClass);
			MethodObject->SetOwner(GetOwner());
			MethodObject->SetSilhouetteComponent(this);
		}
	}
}

FCollisionQueryParams ULXRSilhouetteDetectionComponent::GetCollisionQueryParams(const TArray<AActor*>& ActorsToIgnore) const
{
	FCollisionQueryParams Params(GetOwner()->GetFName(), SCENE_QUERY_STAT_ONLY(KismetTraceUtils), false);

	Params.AddIgnoredActors(ActorsToIgnore);
	return Params;
}




void ULXRSilhouetteDetectionComponent::DoWorkOnSilhouetteTasks()
{
	for (auto It = CurrentSilhouetteTasks.CreateIterator(); It; ++It)
	{
		FSilhouetteTaskData& Task = It.Value();


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
		GEngine->AddOnScreenDebugMessage(uint64(this) + 1, 0.05f, FColor::Magenta, FString::Printf(TEXT("%f :: State %s"), GetComponentTickInterval(), *TaskStateToString(Task.GetTaskState())));

		
		bool IsStateStarted = Task.IsStateStarted();

		switch (Task.GetTaskState())
		{
		case ETaskState::Cleanup:
			if (!IsStateStarted)
			{
				// If no longer valid this tick -> delete task now
				if (!Task.bValidTick)
				{
					It.RemoveCurrent();
					continue;
				}
				Task.NextState();
			}
			break;
		case ETaskState::GeneratingTargets:
			if (!IsStateStarted)
			{
				GenerateSilhouetteTargetsForTask(Task);
			}
			break;
		case ETaskState::Processing:
			if (!IsStateStarted)
			{
				ProcessSilhouetteTask(Task);
			}
			break;
		case ETaskState::Analyze:
			if (!IsStateStarted)
			{
				AnalyzeSilhouetteTask(Task);
			}

			break;
		case ETaskState::Max:
			break;
		}
		
	}

}

void ULXRSilhouetteDetectionComponent::GenerateSilhouetteTargetsForTask(FSilhouetteTaskData& Task)
{
	
	if (const auto AsCharacter = Cast<ACharacter>(Task.DetectionComponent->GetOwner()))
	{
		if (const USkeletalMeshComponent* SkeletalMeshComponent = AsCharacter->GetMesh())
			Task.Bounds = SkeletalMeshComponent->Bounds;
		else
			Task.Bounds = AsCharacter->GetCapsuleComponent()->Bounds;
	}
	else
	{
		FVector Origin, Extent;
		FBoxCenterAndExtent BoxCenterAndExtent;
		Task.DetectionComponent->GetOwner()->GetActorBounds(true, Origin, Extent);
		BoxCenterAndExtent.Center = Origin;
		BoxCenterAndExtent.Extent = Extent;
		Task.Bounds = FBoxSphereBounds(BoxCenterAndExtent.GetBox());
	}

	ULXRFunctionLibrary::FetchSilhouetteTraceParams(*Task.Instigator, *Task.DetectionComponent->GetOwner(), Task);
	if (GetOwner()->Implements<ULXRSilhouette>())
	{
		ILXRSilhouette::Execute_GetSilhouetteTraceLocationAndDirection(GetOwner(), Task.InstigatorViewPointLocation, Task.InstigatorViewPointRotation);
	}
	switch (SilhouetteCalculationType)
	{
	case ETaskProcessType::Sync:
		(new FAutoDeleteAsyncTask<FGenerateSilhouetteTargetsAsyncTask>(Task, SilhouetteTraceTaskParams))->StartSynchronousTask();

		break;

	case ETaskProcessType::Multithread:
		(new FAutoDeleteAsyncTask<FGenerateSilhouetteTargetsAsyncTask>(Task, SilhouetteTraceTaskParams))->StartBackgroundTask();

		break;

	default: ;
	}
}

void ULXRSilhouetteDetectionComponent::ProcessSilhouetteTask(FSilhouetteTaskData& Task)
{
	FRotator CylinderDirection;
	FVector OctreeQueryCenter = Task.DetectionComponent->GetOwner()->GetActorLocation();
	GetSilhouetteCylinderStartAndDirection(*Task.DetectionComponent->GetOwner(), OctreeQueryCenter, CylinderDirection);
	Task.Lights = GetOctreeLights(OctreeQueryCenter);
	
	Task.PassedDatas.Reset();
	
	switch (SilhouetteCalculationType)
	{
	case ETaskProcessType::Sync:
		(new FAutoDeleteAsyncTask<FProcessSilhouetteTaskAsyncTask>(Task))->StartSynchronousTask();
		break;

	case ETaskProcessType::Multithread:
		(new FAutoDeleteAsyncTask<FProcessSilhouetteTaskAsyncTask>(Task))->StartBackgroundTask();
		break;
	}
}


void ULXRSilhouetteDetectionComponent::AnalyzeSilhouetteTask(FSilhouetteTaskData& Task)
{
	Task.StartState();
	if (ULXRFunctionLibrary::GetLuxFromTask(this, Task, DebugOptions))
	{
		// ULXRFunctionLibrary::DebugLXR(this, Task.TargetsLXR, 0.1f, true);
		Task.DetectionComponent->OnSeenBySilhouette.Broadcast(this->GetOwner());
		OnSilhouetteSpottedActor.Broadcast(this->GetOwner());
	}
	Task.NextState(); //cleanup
}

void ULXRSilhouetteDetectionComponent::GetOwnerStartLocationAndRotation(FVector& OutLocation, FRotator& OutRotation) const
{
	if (auto AsCharacter = Cast<ACharacter>(GetOwner()))
	{
		UCapsuleComponent* CapsuleComponent = AsCharacter->GetCapsuleComponent();
		auto Bounds = CapsuleComponent->Bounds;
		OutLocation = Bounds.Origin;
	}
	else
	{
		FVector Extent;
		GetOwner()->GetActorBounds(true, OutLocation, Extent);
	}

	USkeletalMeshComponent* SkeletalMeshComponent = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();


	switch (TraceTransform)
	{
	case ETraceTransform::None:
		break;
	case ETraceTransform::Socket:

		if (SkeletalMeshComponent)
		{
			if (SkeletalMeshComponent->DoesSocketExist(TraceTransformSocket))
			{
				FTransform SocketTransform = SkeletalMeshComponent->GetSocketTransform(TraceTransformSocket);
				OutLocation = SocketTransform.GetLocation();
				OutRotation = SocketTransform.Rotator();
			}
		}
		break;
	case ETraceTransform::Actor:
		{
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
			if (GetOwner()->GetClass()->ImplementsInterface(ULXRSilhouette::StaticClass()))
			{
				ILXRSilhouette::Execute_GetSilhouetteTraceLocationAndDirection(GetOwner(), OutLocation, OutRotation);
			}
		}
		break;
	default: ;
	}
}




void ULXRSilhouetteDetectionComponent::GetSilhouetteCylinderStartAndDirection(const AActor& SilhouetteActor, FVector& SilhouetteLocation, FRotator& Rotator) const
{
	FVector OwnerViewPointLocation;
	FRotator Rotation;
	GetOwnerStartLocationAndRotation(OwnerViewPointLocation, Rotation);
	if (IsValid(&SilhouetteActor))
	{
		const bool OnlyColliding = true;
		FVector Origin, Extent;
		if (auto AsCharacter = Cast<ACharacter>(&SilhouetteActor))
		{
			FBoxSphereBounds Bounds;
			USkeletalMeshComponent* SkeletalMeshComponent = AsCharacter->GetMesh();
			if (SkeletalMeshComponent)
				Bounds = SkeletalMeshComponent->Bounds;
			else
				Bounds = AsCharacter->GetCapsuleComponent()->Bounds;

			Origin = Bounds.Origin;
			Extent = Bounds.BoxExtent;
		}
		else
		{
			SilhouetteActor.GetActorBounds(OnlyColliding, Origin, Extent);
		}
		SilhouetteLocation = Origin;
		if (bDrawDebug)
		{
			DrawDebugBox(GetWorld(), SilhouetteLocation, Extent, FColor::Orange, false, 1.f, 0, 2);
		}
		// DrawDebugSphere(GetWorld(),SilhouetteLocation,50.f,8,FColor::Yellow,false,1.f);
		Rotator = UKismetMathLibrary::MakeRotFromX((SilhouetteLocation - OwnerViewPointLocation).GetSafeNormal());
	}
}

// Called every frame
void ULXRSilhouetteDetectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	DetectionComponentFillTimer += DeltaTime;
	if (DetectionComponentFillTimer > DetectionComponentUpdateCheckInterval)
	{
		if (FMath::RandBool())
		{
			FillNearbyDetectionComponents();
		}
		DetectionComponentFillTimer = 0.f;
	}

	for (auto& Pair : CurrentSilhouetteTasks)
	{
		Pair.Value.bValidTick = false;
	}

	FVector Start;
	FRotator Rotation;
	GetOwnerStartLocationAndRotation(Start, Rotation);

	const FVector Forward = Rotation.Vector();
	const float CosThreshold = FMath::Cos(FMath::DegreesToRadians(TraceAngle));

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(GetOwner());
	TraceParams.bTraceComplex = false;

	for (const TWeakObjectPtr<AActor>& DetectorWeak : NearbyDetectors)
	{
		AActor* Detector = DetectorWeak.Get();
		if (!Detector) continue;

		FVector End = Detector->GetActorLocation() + FVector(0, 0, 50);

		if (ACharacter* AsCharacter = Cast<ACharacter>(Detector))
		{
			End = AsCharacter->GetCapsuleComponent()->Bounds.Origin;
		}

		// Angle check
		{
			const FVector Dir = (End - Start).GetSafeNormal();
			if (FVector::DotProduct(Forward, Dir) <= CosThreshold)
			{
#if UE_ENABLE_DEBUG_DRAWING
				if (bDrawDebug)
				{
					DrawDebugDirectionalArrow(GetWorld(), Start, Start + Dir * 500, 250.f, FColor::Green, false, 0.5f);
					DrawDebugDirectionalArrow(GetWorld(), Start, Start + Forward * 500, 250.f, FColor::Red, false, 0.5f);
				}
#endif
				continue;
			}
		}

		// Occlusion check
		TraceParams.AddIgnoredActor(Detector);
		const bool bBlocked = GetWorld()->LineTraceTestByChannel(Start, End, TraceChannel, TraceParams);
		// If you don't have RemoveIgnoredActor in your UE version, rebuild TraceParams each loop instead.
		// TraceParams.RemoveIgnoredActor(Detector);

		if (bBlocked)
		{
			continue;
		}

		ULXRDetectionComponent* DetectionComponent = Detector->FindComponentByClass<ULXRDetectionComponent>();
		if (!DetectionComponent)
		{
			continue;
		}

		FSilhouetteTaskData* Existing = CurrentSilhouetteTasks.Find(Detector);
		if (Existing)
		{
			Existing->bValidTick = true;
			continue;
		}

		FSilhouetteTaskData& Data = CurrentSilhouetteTasks.Add(Detector);
		Data.bDrawDebug = bDrawDebug;
		Data.TargetsRequired = TargetsRequired;
		Data.TraceTargetBatchCount = TraceTargetBatchCount;
		Data.SilhouetteCalculationType = SilhouetteCalculationType;
		Data.SilhouetteTraceTaskParams = FSilhouetteTraceTaskParams(SilhouetteCheckQuality);
		Data.TraceType = EDetectionType::Silhouette;
		Data.DetectionComponent = DetectionComponent;
		Data.Instigator = GetOwner();
		Data.bValidTick = true;
	}

	DoWorkOnSilhouetteTasks();

}

void ULXRSilhouetteDetectionComponent::FillNearbyDetectionComponents()
{
	OctreeBoundsTestObject.Center = GetOwner()->GetActorLocation();
	OctreeBoundsTestObject.Extent = FVector4(FVector(OctreeCheckBoundsSize));
	LXRSubsystem->GetOctree()->FindElementsWithBoundsTest(OctreeBoundsTestObject, [this](const FLXROctreeElement& Element)
	{
		if (Element.GetOwner() != GetOwner() && (Element.Data->OctreeElementType == Detection || Element.Data->OctreeElementType == Both))

			if (Element.Data.Get().TreeActor.IsValid())
			{
				const TWeakObjectPtr<AActor> DetectionOwner = Element.Data.Get().TreeActor;
				NearbyDetectors.AddUnique(DetectionOwner);
			}
	});
}


TArray<TWeakObjectPtr<ULXRSourceComponent>>& ULXRSilhouetteDetectionComponent::GetOctreeLights(FVector& InLocation)
{
	if (RefreshOctreeLights)
	{
		OctreeLights.Reset();

		OctreeBoundsTestObject.Center = InLocation;
		OctreeBoundsTestObject.Extent = FVector4(FVector(SilhouetteTraceTaskParams.SilhouetteDistance));
		LXRSubsystem = GetOwner()->GetWorld()->GetSubsystem<ULXRSubsystem>();
		bool bSoloFound = LXRSubsystem->bSoloFound;

		LXRSubsystem->GetOctree()->FindElementsWithBoundsTest(OctreeBoundsTestObject, [this,bSoloFound](const FLXROctreeElement& Element)
		{
			if (Element.GetOwner() == GetOwner() || Element.Data->OctreeElementType == Detection) return;

			if (Element.Data.Get().TreeActor.IsValid())
			{
				const TWeakObjectPtr<ULXRSourceComponent> SourceComponent = Element.Data.Get().SourceComponent;

				if (bSoloFound)
				{
					if (SourceComponent->bSolo)
						OctreeLights.AddUnique(SourceComponent);
				}
				else
				{
					OctreeLights.AddUnique(SourceComponent);
				}
			}
		});

		// const float DebugDrawTimeToUse = CheckRate;
		// if (bDrawDebug)
		// {
		// 	DrawDebugDirectionalArrow(GetWorld(), GetOwner()->GetTransform().TransformPosition(RelativeLocation), OctreeBoundsTestObject.Center, 550, FColor::Green, false, DebugDrawTimeToUse / 2, 0, 2);
		//
		// 	GEngine->AddOnScreenDebugMessage(95, DebugDrawTimeToUse, FColor::Magenta, FString::Printf(TEXT("LightSense OctreeLights: %d"), OctreeLights.Num()));
		// 	for (TWeakObjectPtr<AActor> OctreeLight : OctreeLights)
		// 	{
		// 		if (OctreeLight.IsValid())
		// 		{
		// 			DrawDebugDirectionalArrow(GetWorld(), GetOwner()->GetActorLocation(), OctreeLight->GetActorLocation(), 150, FColor::Blue, false, DebugDrawTimeToUse, 0, 2);
		// 		}
		// 	}
		// }
	}

	if (OctreeLights.Num() == 0)
	{
		RefreshOctreeLights = true;
	}

	return OctreeLights;
}

// void ULXRSilhouetteDetectionComponent::ResetSilhouetteArrays()
// {
// 	if (LightLock)
// 	{
// 		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ULXRSilhouetteDetectionComponent::ResetSilhouetteArrays);
// 		return;
// 	}
// 	// GEngine->AddOnScreenDebugMessage(static_cast<int32>(GetUniqueID()), 0.5f, FColor::Magenta, FString::Printf(TEXT("LightSense REMOVE LIGHTS")));
// 	LightLock = true;
// 	CurrentDetectionComponents.Reset();
// 	// AllGeneratedTraceTargets.Reset();
// 	ChosenTraceTargets.Reset();
// 	LightLock = false;
// }
