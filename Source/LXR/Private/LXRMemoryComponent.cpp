// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.


#include "LXRMemoryComponent.h"
#include "DrawDebugHelpers.h"
#include "LXR.h"
#include "Async/Async.h"


// Sets default values for this component's properties
ULXRMemoryComponent::ULXRMemoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void ULXRMemoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}


// Called every frame
void ULXRMemoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...

	if (bDrawDebug)
	{
		for (auto It = MemorizedLights.CreateConstIterator(); It; ++It)
		{
			if (!It.Key().IsValid()) continue;
			float time = GetWorld()->RealTimeSeconds * 5;
			time = (FMath::Sin(time) + 1) * 0.5;
			FVector Offset = FVector(0, 0, 60) * time;
			FVector Location = It.Key()->GetOwner()->GetActorLocation() + Offset;
			FVector Direction =  Location.Z > GetOwner()->GetActorLocation().Z ? FVector::DownVector : FVector::UpVector;
			DrawDebugCone(GetWorld(), Location + Direction * 120, Direction, 50, FMath::DegreesToRadians(15), FMath::DegreesToRadians(15), 6, It.Value() == ELightState::Enabled ? FColor::Green : FColor::Red, false, DeltaTime);
		}
	}
}

void ULXRMemoryComponent::AddOrUpdateLightState(const TWeakObjectPtr<ULXRSourceComponent>& SourceComponent)
{
	if (!SourceComponent.IsValid()) return;
	{
		if (!MemorizedLights.Contains(SourceComponent))
			AddNewMemory(*SourceComponent);
		else
			UpdateMemory(*SourceComponent);
	}
}

void ULXRMemoryComponent::AddOrUpdateLightStateFromThread(const TWeakObjectPtr<ULXRSourceComponent>& LightSourceActor)
{
	FRWScopeLock MemoryLightLock(MemoryDataLockObject, SLT_Write);
	AddOrUpdateLightState(LightSourceActor);
}

void ULXRMemoryComponent::UpdateMemory(ULXRSourceComponent& SourceComponent)
{
	ELightState OldState = MemorizedLights[&SourceComponent];
	ELightState NewState = SourceComponent.GetLightState();


	if (NewState != OldState)
	{
		if (bPrintDebug)
		{
			UE_LOG(LogLXR, Warning, TEXT("MemoryOn: %s | %s %s -> %s" ), *GetOwner()->GetName(), *SourceComponent.GetOwner()->GetName(), *StaticEnum<ELightState>()->GetValueAsString(OldState), *StaticEnum<ELightState>()->GetValueAsString(NewState));
		}

		MemorizedLights[&SourceComponent] = NewState;
		BroadcastLightStateChange(SourceComponent, OldState, NewState);
	}

	for (const auto Tuple : MemorizedLightComponents[&SourceComponent])
	{
		const ULightComponent* Component = SourceComponent.GetMyLightComponents()[Tuple.Key];
		OldState = MemorizedLightComponents[&SourceComponent][Tuple.Key];
		NewState = SourceComponent.GetLightComponentState(Component);

		if (NewState != OldState)
		{
			MemorizedLightComponents[&SourceComponent][Tuple.Key] = NewState;
			BroadcastLightComponentStateChange(SourceComponent, *Component, OldState, NewState);
		}
	}
}

void ULXRMemoryComponent::RemoveMemory(const TWeakObjectPtr<ULXRSourceComponent>& LightSourceActor)
{
	if (MemorizedLights.Contains(LightSourceActor))
	{
		MemorizedLights.Remove(LightSourceActor);
		MemorizedLightComponents.Remove(LightSourceActor);
	}
}


ELightState ULXRMemoryComponent::GetStateOfMemorizedLight(ULXRSourceComponent* SourceComponent)
{
	if (MemorizedLights.Contains(SourceComponent))
		return MemorizedLights[SourceComponent];

	return ELightState::None;
}

void ULXRMemoryComponent::AddNewMemory(ULXRSourceComponent& SourceComponent)
{
	MemorizedLights.Add(&SourceComponent);
	MemorizedLightComponents.Add(&SourceComponent);

	MemorizedLights[&SourceComponent] = SourceComponent.GetLightState();
	for (const auto Tuple : MemorizedLightComponents[&SourceComponent])
	{
		const ULightComponent* Component = SourceComponent.GetMyLightComponents()[Tuple.Key];
		MemorizedLightComponents[&SourceComponent][Tuple.Key] = SourceComponent.GetLightComponentState(Component);
	}

	const ELightState NewState = SourceComponent.GetLightState();
	BroadcastLightStateChange(SourceComponent, ELightState::None, NewState);

	if (bPrintDebug)
	{
		UE_LOG(LogLXR, Warning, TEXT("New Memory Added: %s | %s :: %s " ), *GetOwner()->GetName(), *SourceComponent.GetOwner()->GetName(), *StaticEnum<ELightState>()->GetValueAsString(NewState));
	}
}

void ULXRMemoryComponent::BroadcastLightStateChange(const ULXRSourceComponent& SourceComponent, ELightState OldState, ELightState NewState) const
{
	if (!IsInGameThread())
	{
		TWeakObjectPtr<const ULXRMemoryComponent> WeakSelf(this);
		TWeakObjectPtr<const ULXRSourceComponent> WeakSource(const_cast<ULXRSourceComponent*>(&SourceComponent));
		AsyncTask(ENamedThreads::GameThread, [WeakSelf, WeakSource, OldState, NewState]()
		{
			if (!WeakSelf.IsValid()) return;
			if (WeakSelf->OnMemorizedLightStateChanged.IsBound())
			{
				WeakSelf->OnMemorizedLightStateChanged.Broadcast(WeakSource.Get(), OldState, NewState);
			}
		});
	}
	else
	{
		OnMemorizedLightStateChanged.Broadcast(&SourceComponent, OldState, NewState);
	}
}

void ULXRMemoryComponent::BroadcastLightComponentStateChange(const ULXRSourceComponent& SourceComponent, const ULightComponent& LightComponent, ELightState OldState, ELightState NewState) const
{
	if (OnMemorizedLightComponentStateChanged.IsBound())
	{
		OnMemorizedLightComponentStateChanged.Broadcast(&SourceComponent, &LightComponent, OldState, NewState);
	}
}
