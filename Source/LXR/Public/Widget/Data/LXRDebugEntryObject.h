// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "LXRFluxLightDetector.h"
#include "LXR_Shared.h"
#include "UObject/Object.h"
#include "LXRDebugEntryObject.generated.h"

enum class ELXRDebugFeature : uint8;
class ULXRLightSenseComponent;
class ULXRDetectionComponent;
struct FLXRDebugFeatures;
/**
 * 
 */
UCLASS()
class LXR_API ULXRDebugEntryObject : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AActor* InDebugActor, const FLXRDebugFeatures& InDebugFeatures);

	UFUNCTION(BlueprintPure, Category = "LXR|Debug")
	AActor* GetMyDebugActor() const;

	TSharedPtr<FLXRDebugFeatures> MyDebugFeatures;
	FOnDebugFeaturesChanged DebugSettingsChanged;

	UPROPERTY(BlueprintReadOnly, Category = "LXR|Debug")
	FLinearColor DebugColor;

	void SetComponent(ELXRDebugFeature InDebugFeature, UActorComponent* InComponent);
	TObjectPtr<UActorComponent> GetMyComponent(ELXRDebugFeature InDebugFeature);


private:

	UPROPERTY()
	TObjectPtr<ULXRDetectionComponent> DetectionComponent;
	UPROPERTY()
	TObjectPtr<ULXRLightSenseComponent> LightSenseComponent;
	UPROPERTY()
	TObjectPtr<ULXRFluxLightDetectorComponent> FluxLightDetectorComponent;
};
