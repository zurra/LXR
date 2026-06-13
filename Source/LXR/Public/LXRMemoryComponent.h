// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "LXRSourceComponent.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "LXRMemoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMemorizedLightStateChanged, const ULXRSourceComponent*, LightSourceComponent, ELightState, OldLightState, ELightState, NewLightState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnMemorizedLightComponentStateChanged, const ULXRSourceComponent*, LightSourceComponent, const ULightComponent*, LightComponent, ELightState, OldLightComponentState,
                                              ELightState, NewLightComponentState);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LXR_API ULXRMemoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	ULXRMemoryComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	//Render debug shapes.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Memory|Debug")
	bool bDrawDebug = false;

	//Print debug messages.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Memory|Debug")
	bool bPrintDebug = false;


	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void AddOrUpdateLightState(const TWeakObjectPtr<ULXRSourceComponent>& SourceComponent);
	void AddOrUpdateLightStateFromThread(const TWeakObjectPtr<ULXRSourceComponent>& LightSourceActor);
	void RemoveMemory(const TWeakObjectPtr<ULXRSourceComponent>& LightSourceActor);


	/**
	* Gets current memory state of a light.
	* @return Current memory state of a light.
	*/
	UFUNCTION(BlueprintCallable, Category="LXR|Memory")
	ELightState GetStateOfMemorizedLight(ULXRSourceComponent* SourceComponent);

	/**
	Broadcasts when any Light Component state changes.
	Blueprint Assignable event

	@return LightSourceComponent : Changed Source Component
	@return OldLightState : Old state
	@return NewLightState : New state
	*/
	UPROPERTY(BlueprintAssignable, Category="LXR|Memory")
	FOnMemorizedLightStateChanged OnMemorizedLightStateChanged;

	/**
	Broadcasts when Source Component state changes.
	Blueprint Assignable event
	@return LightSourceComponent : Changed Source Component
	@return LightComponent : Changed Light component on Source actor
	@return OldLightComponentState : Old state
	@return NewLightComponentStat : New state
	*/
	UPROPERTY(BlueprintAssignable, Category="LXR|Memory")
	FOnMemorizedLightComponentStateChanged OnMemorizedLightComponentStateChanged;

	//Which class to use for Memory checks, either Detection or Sense.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Memory")
	EMemoryCheckClass MemoryCheckClass = EMemoryCheckClass::Detection;

private:
	TMap<TWeakObjectPtr<ULXRSourceComponent>, ELightState> MemorizedLights;
	TMap<TWeakObjectPtr<ULXRSourceComponent>, TMap<int, ELightState>> MemorizedLightComponents;

	mutable FRWLock MemoryDataLockObject;

	void AddNewMemory(ULXRSourceComponent& SourceComponent);
	void UpdateMemory(ULXRSourceComponent& SourceComponent);
	void BroadcastLightStateChange(const ULXRSourceComponent& SourceComponent, ELightState OldState, ELightState NewState) const;
	void BroadcastLightComponentStateChange(const ULXRSourceComponent& SourceComponent, const ULightComponent& LightComponent, ELightState OldState, ELightState NewState) const;
};
