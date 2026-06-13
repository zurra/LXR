// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#include "LXRSourceComponent.h"
#include "Async/Async.h"
#include "ILXRSource.h"
#include "LXRFunctionLibrary.h"
#include "LXRMethodObject.h"
#include "LXRSubsystem.h"
#include "Components/DirectionalLightComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Async/ParallelFor.h"

// Sets default values for this component's properties
ULXRSourceComponent::ULXRSourceComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	if (!bAlwaysRelevant)
	{
		PrimaryComponentTick.bCanEverTick = true;
		PrimaryComponentTick.bStartWithTickEnabled = true;
		SetComponentTickInterval(1.0f);
	}

	// ...
}

void ULXRSourceComponent::TickComponent(float DeltaTime, ELevelTick Tick, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, Tick, ThisTickFunction);
	if ((LastOctreeLocation - GetOwner()->GetActorLocation()).Size() > 100.f)
	{
		UpdateOctreeElement();
	}
	constexpr float Mpl = 2;
	const float SpeedPercent = FMath::Abs(FMath::Max(1.f, static_cast<float>(GetOwner()->GetVelocity().Size())) / 150 - 1) * Mpl;
	SetComponentTickInterval(FMath::Clamp(SpeedPercent, 0.15f, Mpl));
}


bool ULXRSourceComponent::IsEnabled() const
{
	if (bDisable) return false;

	switch (IsEnabledMethodToUse)
	{
	case EMethodToUse::None:
		break;
	case EMethodToUse::Class:
		for (const ULightComponent* LightComponent : GetMyLightComponents())
		{
			if (IsLightComponentEnabled(LightComponent))
				return true;
		}
		break;
	case EMethodToUse::Interface:
		if (GetOwner() && GetOwner()->Implements<ULXRSource>())
			return ILXRSource::Execute_IsEnabled(GetOwner());
		break;
	case EMethodToUse::UObject:
		if (IsValid(MethodObject))
			return ILXRSource::Execute_IsEnabled(MethodObject);
		break;
	default: ;
	}

	return false;
}


bool ULXRSourceComponent::IsLightComponentEnabled(const ULightComponent* LightComponent) const
{
	if (!IsValid(LightComponent)) return false;
	switch (IsLightComponentEnabledMethodToUse)
	{
	case EMethodToUse::None:
		break;

	case EMethodToUse::Class:
		return LightComponent->IsVisible() && LightComponent->Intensity > 0;

	case EMethodToUse::Interface:
		if (GetOwner()->Implements<ULXRSource>())
			return ILXRSource::Execute_IsLightComponentEnabled(GetOwner(), LightComponent);
		break; 
	case EMethodToUse::UObject:
		if (IsValid(MethodObject))
			return ILXRSource::Execute_IsLightComponentEnabled(MethodObject, LightComponent);

	default: ;
	}
	return false;
}

TArray<AActor*> ULXRSourceComponent::GetIgnoreVisibilityActors_Implementation()
{
	switch (IgnoreVisibilityActorsMethodToUse)
	{
	case EMethodToUse::None:
		break;

	case EMethodToUse::Class:
		return IgnoreVisibilityActors;

	case EMethodToUse::Interface:
		if (GetOwner()->Implements<ULXRSource>())
			IgnoreVisibilityActors = ILXRSource::Execute_GetIgnoreVisibilityActors(GetOwner());

	case EMethodToUse::UObject:
		{
			TWeakObjectPtr<ULXRSourceComponent> WeakSelf(this);
			AsyncTask(ENamedThreads::GameThread, [WeakSelf]()
			{
				ULXRSourceComponent* Self = WeakSelf.Get();
				if (!Self) return;
				if (IsValid(Self->MethodObject))
					Self->IgnoreVisibilityActors = ILXRSource::Execute_GetIgnoreVisibilityActors(Self->MethodObject);
			});
		}


	default: ;
	}
	return IgnoreVisibilityActors;
}

ELightState ULXRSourceComponent::GetLightState() const
{
	switch (GetSourceActorStateMethodToUse)
	{
	case EMethodToUse::None:
		break;
	case EMethodToUse::Class:
		if (IsEnabled())
			return ELightState::Enabled;
		break;
	case EMethodToUse::Interface:
		if (GetOwner()->Implements<ULXRSource>())
			return ILXRSource::Execute_GetLightActorState(GetOwner());
		break;
	case EMethodToUse::UObject:
		if (IsValid(MethodObject))
			return ILXRSource::Execute_GetLightActorState(MethodObject);
		break;
	default: ;
	}

	return ELightState::Disabled;
}


ELightState ULXRSourceComponent::GetLightComponentState(const ULightComponent* InComponent) const
{
	switch (GetLightComponentStateMethodToUse)
	{
	case EMethodToUse::None:
		break;
	case EMethodToUse::Class:
		if (GetMyLightComponents().Contains(InComponent))
		{
			if (IsLightComponentEnabled(InComponent))
				return ELightState::Enabled;
			else
				return ELightState::Disabled;
		}
		break;
	case EMethodToUse::Interface:
		if (GetOwner()->Implements<ULXRSource>())
			return ILXRSource::Execute_GetLightComponentState(GetOwner(), InComponent);
		break;
	case EMethodToUse::UObject:
		if (IsValid(MethodObject))
			return ILXRSource::Execute_GetLightComponentState(MethodObject, InComponent);
		break;
	default:
		break;
	}
	return ELightState::None;
}

FLinearColor ULXRSourceComponent::GetLightComponentColor(const ULightComponent& LightComponent)
{
	FLinearColor LightColor;
	if (LightComponent.bUseTemperature)
	{
		LightColor = FLinearColor::MakeFromColorTemperature(LightComponent.Temperature);
		LightColor *= LightComponent.GetLightColor();
	}
	else
		LightColor = LightComponent.GetLightColor();

	return LightColor;
}


FLinearColor ULXRSourceComponent::GetCombinedColors()
{
	TArray<FLinearColor> CombinedLightColors;
	for (auto It = GetMyLightComponents().CreateConstIterator(); It; ++It)
	{
		const ULightComponent* LightComponent = *It;
		FLinearColor LightColor;
		LightColor = GetLightComponentColor(*LightComponent);
		CombinedLightColors.Add(LightColor);
	}

	return ULXRFunctionLibrary::GetLinearColorArrayAverage(CombinedLightColors);
}

FLinearColor ULXRSourceComponent::GetCombinedColorsByComponents(const TArray<ULightComponent*>& LightComponents)
{
	TArray<FLinearColor> CombinedLightColors;
	for (const ULightComponent* const LightComponent : LightComponents)
	{
		FLinearColor LightColor;
		LightColor = GetLightComponentColor(*LightComponent);
		CombinedLightColors.Add(LightColor);
	}

	return ULXRFunctionLibrary::GetLinearColorArrayAverage(CombinedLightColors);
}

FLinearColor ULXRSourceComponent::GetCombinedColorsByComponentIndices(const TArray<int>& Indices)
{
	TArray<FLinearColor> CombinedLightColors;
	for (const int Idx : Indices)
	{
		FLinearColor LightColor;
		LightColor = GetLightComponentColor(*GetMyLightComponents()[Idx]);
		CombinedLightColors.Add(LightColor);
	}

	return ULXRFunctionLibrary::GetLinearColorArrayAverage(CombinedLightColors);
}


ELightState ULXRSourceComponent::GetIsLightLodEnabled_Implementation()
{
	ELightState ReturnState = CurrentLODState;
	switch (GetSourceActorStateMethodToUse)
	{
	case EMethodToUse::None:
		break;
	case EMethodToUse::Class:
		{
			ULXRSubsystem* LightDetectionSubsystem = GetOwner()->GetWorld()->GetSubsystem<ULXRSubsystem>();
			if (IsValid(LightDetectionSubsystem))
			{
				bool StateChangedToTrue = false;
				int Count = MyLightComponents.Num();
				TArray<ULightComponent*> Lights = MyLightComponents;
				for (int32 index = 0; index < Count; ++index)
				{
					if (Lights.IsValidIndex(index))
					{
						ULightComponent* LightComponent = Lights[index];

						if (IsLightComponentEnabled(LightComponent))
						{
							FVector LightLocation = LightComponent->GetComponentLocation();
							const TTuple<FVector*, FRotator*>* PlayerCameraLocationAndRotation = LightDetectionSubsystem->GetPlayerCameraLocationAndRotation();
							const UE::Math::TVector<double> DistanceAndDirectionToLightComponent = LightLocation - *PlayerCameraLocationAndRotation->Key;
							const auto DistanceToLightComponent = DistanceAndDirectionToLightComponent.Length();
							float Attenuation = 0;
							if (const ULocalLightComponent* LocalLightComponent = Cast<ULocalLightComponent>(LightComponent))
							{
								Attenuation = LocalLightComponent->AttenuationRadius;
							}

							if (DistanceToLightComponent > (Attenuation * AttenuationMultiplierToBeRelevant))
							{
								if (!StateChangedToTrue)
								{
									if (GetWorld()->LineTraceTestByChannel(*PlayerCameraLocationAndRotation->Key, LightLocation, ECC_Visibility))
									{
										ReturnState = ELightState::Disabled;
									}
								}
							}

							if (ReturnState == ELightState::Disabled && !StateChangedToTrue)
							{
								StateChangedToTrue = true;
								ReturnState = ELightState::Enabled;
							}
						}
					}
				}
			}
		}
		break;
	case EMethodToUse::Interface:
		if (GetOwner()->Implements<ULXRSource>())
			return ILXRSource::Execute_GetIsLightLodEnabled(GetOwner());
		break;
	case EMethodToUse::UObject:
		if (IsValid(MethodObject))
			return ILXRSource::Execute_GetIsLightLodEnabled(MethodObject);
		break;
	default: ;
	}

	return ReturnState;
}

void ULXRSourceComponent::OnLodStateChange(bool StateChanged)
{
	ELightState OldState = CurrentLODState;
	CurrentLODState = StateChanged ? ELightState::Enabled : ELightState::Disabled;

	OnLightLodStateChanged.Broadcast(OldState, CurrentLODState);
}

// Called when the game starts
void ULXRSourceComponent::BeginPlay()
{
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypeQueries;
	ObjectTypeQueries.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));

	TArray<AActor*> RawPtrs;
	UKismetSystemLibrary::SphereOverlapActors(this, GetOwner()->GetActorLocation(), OverlapDetectionDistance, ObjectTypeQueries,NULL, {}, RawPtrs);

	MyOverlappingActors = ULXRFunctionLibrary::RawPtrToSmartPtr_Array(RawPtrs);

	if (RawPtrs.Num() > 0)
	{
		UE_LOG(LogLXR, VeryVerbose, TEXT("%hs Light %s Has overlapped stuff nice!"), __FUNCTION__, *GetOwner()->GetName());

		TArray<TObjectPtr<AActor>> ActorsToRemove;
		for (TObjectPtr<AActor> Actor : MyOverlappingActors)
		{
			if (Actor->ActorHasTag(TEXT("LXRCol")))
			{
				UE_LOG(LogLXR, VeryVerbose, TEXT("%hs Actor %s Has tag!"), __FUNCTION__, *GetOwner()->GetName());
				ActorsToRemove.Add(Actor);
			}
		}
		for (TObjectPtr<AActor> Actor : ActorsToRemove)
		{
			MyOverlappingActors.Remove(Actor);
		}
	}

	MyLightComponents = FindMyLightComponents();

	if (IsEnabledMethodToUse == EMethodToUse::UObject
		|| GetSourceActorStateMethodToUse == EMethodToUse::UObject
		|| GetLightComponentStateMethodToUse == EMethodToUse::UObject
		|| IsLightComponentEnabledMethodToUse == EMethodToUse::UObject
		|| GetMyLightComponentsMethodToUse == EMethodToUse::UObject
		|| IgnoreVisibilityActorsMethodToUse == EMethodToUse::UObject
		|| LODDetectionMethodToUse == EMethodToUse::UObject)
	{
		if (LXRMethodObject.IsValid())
		{
			const UClass* MethodObjectClass = LXRMethodObject.TryLoadClass<ULXRMethodObject>();

			// todo this some time....
			// ULXRSubsystem* LXRSubSystem = GetOwner()->GetWorld()->GetSubsystem<ULXRSubsystem>();
			// MethodObject = LXRSubSystem->GetOrAddMethodObject(MethodObjectClass);

			MethodObject = NewObject<ULXRMethodObject>(GetOwner(), MethodObjectClass);
			MethodObject->SetOwner(GetOwner());
			MethodObject->SetSourceComponent(this);
		}
	}


	for (const auto Component : MyLightComponents)
	{
		if (!bAlwaysRelevant)
			bAlwaysRelevant = Component->IsA(UDirectionalLightComponent::StaticClass()) ? true : bAlwaysRelevant;
	}

	RegisterLight();

	if (bBroadcastStateChanges)
	{
		GetWorld()->GetTimerManager().SetTimer(StateChangeTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]
		{
			if (const ELightState NewState = GetLightState(); NewState != CurrentLightState)
			{
				OnLightStateChanged.Broadcast(CurrentLightState, NewState);
				CurrentLightState = NewState;
			}
		}), 0.1f, true);
	}

	Super::BeginPlay();
}

void ULXRSourceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
		GetWorld()->GetTimerManager().ClearTimer(StateChangeTimerHandle);
	UnregisterLight();
	Super::EndPlay(EndPlayReason);
}

void ULXRSourceComponent::DestroyComponent(bool bPromoteChildren)
{
	UnregisterLight();
	Super::DestroyComponent(bPromoteChildren);
}

void ULXRSourceComponent::UpdateOctreeElement()
{
	ULXRSubsystem* LightDetectionSubsystem = GetOwner()->GetWorld()->GetSubsystem<ULXRSubsystem>();
	if (IsValid(LightDetectionSubsystem))
		LastOctreeLocation = LightDetectionSubsystem->UpdateElement(GetOwner());
}

void ULXRSourceComponent::RegisterLight()
{
	ULXRSubsystem* LightDetectionSubsystem = GetOwner()->GetWorld()->GetSubsystem<ULXRSubsystem>();
	if (IsValid(LightDetectionSubsystem))
		LightDetectionSubsystem->RegisterLight(this);

	LastOctreeLocation = GetOwner()->GetActorLocation();
}


void ULXRSourceComponent::UnregisterLight()
{
	ULXRSubsystem* LightDetectionSubsystem = GetOwner()->GetWorld()->GetSubsystem<ULXRSubsystem>();
	if (IsValid(LightDetectionSubsystem))
	{
		if (LightDetectionSubsystem->GetAllLights().Contains(this))
			LightDetectionSubsystem->UnregisterLight(this);
		else
		{
			UE_LOG(LogLXR, VeryVerbose, TEXT("%hs Light %s not registered with LXR system! \n This warning should not happen but it can happen..."), __FUNCTION__, *GetOwner()->GetName());
		}
	}
}

TArray<TObjectPtr<AActor>>& ULXRSourceComponent::GetMyOverlappingActors()
{
	return MyOverlappingActors;
}

const TArray<ULightComponent*>& ULXRSourceComponent::GetMyLightComponents() const
{
	bool NotValidLights = MyLightComponents.ContainsByPredicate([](const ULightComponent* Light)
	{
		return !IsValid(Light);
	});
	if (MyLightComponents.Num() == 0 || NotValidLights)
		const_cast<ULXRSourceComponent*>(this)->MyLightComponents = FindMyLightComponents();
	return MyLightComponents;
}

TArray<ULightComponent*> ULXRSourceComponent::FindMyLightComponents() const
{
	switch (GetMyLightComponentsMethodToUse)
	{
	case EMethodToUse::None:
		break;
	case EMethodToUse::Class:
		{
			TArray<ULightComponent*> LightComponents;
			if (GetOwner() == nullptr) return {};
			GetOwner()->GetComponents<ULightComponent>(LightComponents);
			for (FComponentReference ExcludedLightComponent : ExcludedLights)
			{
				for (int i = LightComponents.Num() - 1; i >= 0; --i)
				{
					if (LightComponents[i] == ExcludedLightComponent.GetComponent(GetOwner()))
					{
						LightComponents.RemoveAt(i);
					}
				}
			}
			return LightComponents;
		}
		break;

	case EMethodToUse::Interface:
		if (GetOwner()->Implements<ULXRSource>())
			return ILXRSource::Execute_GetMyLightComponents(GetOwner());
		break;
	case EMethodToUse::UObject:
		if (IsValid(MethodObject))
		{
			return ILXRSource::Execute_GetMyLightComponents(MethodObject);
		}
		break;
	default: ;
	}
	return {};
}
