// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ULXRDebugFeatureContainer.generated.h"

class UULXRDebugEntry;
class UULXRDebugFeature;
/**
 * 
 */
UCLASS()
class LXR_API UULXRDebugFeatureContainer : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget), EditAnywhere, Category = "LXR|Debug")
	TObjectPtr<UULXRDebugFeature> DirectDebug;

	UPROPERTY(meta = (BindWidget), EditAnywhere, Category = "LXR|Debug")
	TObjectPtr<UULXRDebugFeature> SenseDebug;

	UPROPERTY(meta = (BindWidget), EditAnywhere, Category = "LXR|Debug")
	TObjectPtr<UULXRDebugFeature> FluxDebug;

	void DebugFeaturesChanged();
	void TickWidgets() const;
	void PrepareWidgets() const;
	void SetDebugEntryWidget(UULXRDebugEntry& InEntryWidget);
};
