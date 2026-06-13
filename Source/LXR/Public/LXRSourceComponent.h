// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "LXR_Shared.h"
#include "Components/ActorComponent.h"
#include "LXRSourceComponent.generated.h"

UENUM()
enum class EMethodToUse
{
	None UMETA(HIDDEN),
	// Class
	Class UMETA(DisplayName = "Class"),
	// Interface
	Interface UMETA(DisplayName = "Interface"),
	// UObject
	UObject UMETA(DisplayName = "UObject")
};

UENUM(BlueprintType)
enum class EMemoryCheckClass : uint8
{
	None UMETA(HIDDEN),
	// Detection.
	Detection UMETA(DisplayName = "Detection"),
	// Sense.
	Sense UMETA(DisplayName = "Sense"),
};

UENUM()
enum class EMemoryDetectionCheckType
{
	None UMETA(HIDDEN),
	// Relevant & Visibility.
	RelevantAndVisibility UMETA(DisplayName = "Relevant & Visibility."),
	// Visibility.
	Visibility UMETA(DisplayName = "Only Visibility."),
};


UENUM(BlueprintType)
enum class ELightState : uint8
{
	None UMETA(HIDDEN),
	// Enabled.
	Enabled UMETA(DisplayName = "Enabled"),
	// Disabled.
	Disabled UMETA(DisplayName = "Disabled"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLightStateChanged, ELightState, OldLightState, ELightState, NewLightState);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LXR_API ULXRSourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	ULXRSourceComponent();
	
	//Possibility to override DetectionComponent Trace channel.
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Visibility")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_MAX;
	
	//Defines the distance within which static actors are considered part of this Source Component.
	//Actors within this distance will be ignored when checking for visibility.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Source")
	float OverlapDetectionDistance = 30.f;

	//Disables detection from all other non-solo LXR Sources
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Source|Debug", meta=(AdvancedDisplay))
	bool bSolo = false;

	//Disables detection from all other non-solo LXR Sources
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Source", meta=(AdvancedDisplay))
	bool bDisable = false;


	//Should this light source actor be always relevant to Light Detection.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Source")
	bool bAlwaysRelevant = false;

	//Add detected actors to list. DetectionComponent bAddToSourceWhenDetected needs to be true.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Source")
	bool bAddDetected = false;

	//This actor can be sensed by light sense component
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Sense")
	bool bEnableLightSense = false;

	//This actor can be memorized by memory  component
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Memory")
	bool bIsMemorizable = false;

	//When enabled, SourceComponent will broadcast Light State changes.
	// OnLightStateChanged event can be implemented in BP or c++.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LXR|Memory")
	bool bBroadcastStateChanges = false;

	//Attenuation multiplier to be relevant.
	//Light is relevant if Actor is closer than Attenuation * AttenuationMultiplierToBeRelevant.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Source")
	float AttenuationMultiplierToBeRelevant = 1;

	//LXR multiplier.
	//Multiplier to add to final LXR.
	//Added to all LightComponents unless overriden with LightLXRMultipliers variable.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Source")
	float LXRMultiplier = 1;

	//LXR color multiplier.
	//Multiplier to add to final LXR color.
	//Added to all LightComponents unless overriden with LightLXRColorMultipliers variable.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Source")
	float LXRColorMultiplier = 1;

	//LXR multiplier for Sense.
	//Multiplier to add to Sense LXR.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Source")
	float LXRSenseMultiplier = 1;

	//LXR multiplier for Silhouette.
	//Multiplier to add to Silhouette LXR.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Source")
	float LXRSilhouetteMultiplier = 10;

	//LXR Intensity multiplier per LightComponent.
	//Overrides LXR Multiplier for light contained in array.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Source")
	TArray<FLightSourceData> LightLXRMultipliers;

	//LXR Color multiplier per LightComponent.
	// Overrides LXR Color Multiplier for light contained in array.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Source")
	TArray<FLightSourceData> LightLXRColorMultipliers;

	//List of LightComponents to exclude from LightDetection, Only used when GetMyLightComponentsMethodToUse is set to Class.
	UPROPERTY(EditAnywhere, Category="LXR|Source",
		meta=(UseComponentPicker, AllowedClasses="/Script/Engine.LightComponent", EditCondition = "GetMyLightComponentsMethodToUse == EMethodToUse::Class"))
	TArray<FComponentReference> ExcludedLights;

	//Method to use for is actor light enabled
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Source")
	EMethodToUse IsEnabledMethodToUse = EMethodToUse::Class;

	//Method to use for actor light state
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Source")
	EMethodToUse GetSourceActorStateMethodToUse = EMethodToUse::Class;

	//Method to use for light component state
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Source")
	EMethodToUse GetLightComponentStateMethodToUse = EMethodToUse::Class;

	//Method to use for if actor light component is enabled.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Source")
	EMethodToUse IsLightComponentEnabledMethodToUse = EMethodToUse::Class;

	//Method to use for get my lights components.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Source")
	EMethodToUse GetMyLightComponentsMethodToUse = EMethodToUse::Class;

	//Method to use for ignore visibility actors.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Source")
	EMethodToUse IgnoreVisibilityActorsMethodToUse = EMethodToUse::Class;

	//Method to use for ignore visibility actors.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Source")
	EMethodToUse LODDetectionMethodToUse = EMethodToUse::Class;

	//Assign the LXRMethodObject to use which overrides the specified method.
	// If any of the Method To Use properties are set to UObject then field LXRMethodObject can be set.
	UPROPERTY(EditAnywhere, Category="LXR|Source",
		meta=(UseComponentPicker, MetaClass="/Script/LXR.LXRMethodObject", BlueprintBaseOnly, EditCondition =
			"IsEnabledMethodToUse == EMethodToUse::UObject || GetSourceActorStateMethodToUse == EMethodToUse::UObject || GetLightComponentStateMethodToUse == EMethodToUse::UObject || IsLightComponentEnabledMethodToUse == EMethodToUse::UObject || GetMyLightComponentsMethodToUse == EMethodToUse::UObject || IgnoreVisibilityActorsMethodToUse == EMethodToUse::UObject || LODDetectionMethodToUse == EMethodToUse::UObject "
		))
	FSoftClassPath LXRMethodObject;

	//List of Detected actors.
	UPROPERTY(BlueprintReadWrite, Category="LXR|Source")
	TArray<TObjectPtr<AActor>> DetectedActors;

	//List of actors to ignore when checking visibility.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Source")
	TArray<TObjectPtr<AActor>> IgnoreVisibilityActors;

	//IF owner does not implement ILightSource::IsEnabled, then use this function to determine if light source is enabled.
	UFUNCTION(BlueprintPure, Category="LXR|Source")
	bool IsEnabled() const;

	//IF owner does not implement ILightSource::IsLightComponentEnabled, then use this function to determine if light component is enabled.
	UFUNCTION(BlueprintPure, Category="LXR|Source")
	bool IsLightComponentEnabled(const ULightComponent* LightComponent) const;

	UFUNCTION(BlueprintPure, Category="LXR|Source")
	ELightState GetLightState() const;

	UFUNCTION(BlueprintPure, Category="LXR|Source")
	ELightState GetLightComponentState(const ULightComponent* InComponent) const;

	UFUNCTION(BlueprintPure, Category="LXR|Source")
	FLinearColor GetCombinedColors();

	UFUNCTION(BlueprintPure, Category="LXR|Source")
	FLinearColor GetCombinedColorsByComponents(const TArray<ULightComponent*>& LightComponents);

	UFUNCTION(BlueprintPure, Category="LXR|Source")
	FLinearColor GetCombinedColorsByComponentIndices(const TArray<int>& Indices);

	//Get actors to ignore when checking visibility.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="LXR|Source")
	TArray<AActor*> GetIgnoreVisibilityActors();

	//Get is light lod system enabled.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="LXR|Source")
	ELightState GetIsLightLodEnabled();


	void OnLodStateChange(bool StateChanged);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void DestroyComponent(bool bPromoteChildren) override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FLinearColor GetLightComponentColor(const ULightComponent& LightComponent);

public:
	void RegisterLight();
	void UnregisterLight();
	void UpdateOctreeElement();

	const TArray<ULightComponent*>& GetMyLightComponents() const;
	TArray<TObjectPtr<AActor>>& GetMyOverlappingActors();

	// TMap<ULightComponent*,ELightState> LightComponentStates;

	FVector LastOctreeLocation;

protected:
	UPROPERTY(BlueprintReadOnly, Category="LXR|Source")
	TArray<TObjectPtr<ULightComponent>> MyLightComponents;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> MyOverlappingActors;

	UPROPERTY()
	TObjectPtr<class ULXRMethodObject> MethodObject;

	TArray<ULightComponent*> FindMyLightComponents() const;

	ELightState CurrentLightState;

	ELightState CurrentLODState;

	FTimerHandle StateChangeTimerHandle;

	UPROPERTY(BlueprintAssignable, Category="LXR|Memory")
	FOnLightStateChanged OnLightStateChanged;

	UPROPERTY(BlueprintAssignable, Category="LXR|LOD")
	FOnLightStateChanged OnLightLodStateChanged;
};
