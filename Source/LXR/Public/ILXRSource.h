// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ILXRSource.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class ULXRSource : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class LXR_API ILXRSource
{
	GENERATED_BODY()
	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	
	/* Is light actor enabled.
	Disabled light actor can be relevant to light detection but will not be traced against. */ 
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="LXR|Source")
	bool IsEnabled();

	bool IsEnabled_Implementation();
	
	/* Is certain light component enabled.
	Disabled light component can be relevant to light detection but will not be traced against. */ 
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="LXR|Source")
	bool IsLightComponentEnabled(const ULightComponent* LightComponent);
	
	/* Get actor's light components.
	By default ULXRLightSource will return all LightComponents for detection. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="LXR|Source")
	TArray<ULightComponent*> GetMyLightComponents();
	
	/* Get actor's light state.
	Used by memory system */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="LXR|Source")
	ELightState GetLightActorState();

	/* Get actor's light component's state.
	Used by memory system */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="LXR|Source")
	ELightState GetLightComponentState(const ULightComponent* LightComponent);

	/* Get actor's IgnoreVisibilityActors.
	List of actors to ignore when checking visibility. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="LXR|Source")
	TArray<AActor*> GetIgnoreVisibilityActors();

	/* Get actor's IgnoreVisibilityActors.
List of actors to ignore when checking visibility. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="LXR|Source")
	ELightState GetIsLightLodEnabled();
	
	
		
};
