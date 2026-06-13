// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "LXRDebugSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "ULXRDebugFeature.generated.h"

/**
 * 
 */
UCLASS()
class LXR_API UULXRDebugFeature : public UUserWidget
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintImplementableEvent, Category = "LXR|Debug")
	void BP_WidgetReady();

	UFUNCTION(BlueprintImplementableEvent, Category = "LXR|Debug")
	void CustomTick();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LXR|Debug")
	AActor* GetMyDebugActor() const;

	UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = "FeatureComponent"), Category = "LXR|Debug")
	UActorComponent* GetMyComponent(TSubclassOf<UActorComponent> FeatureComponent) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LXR|Debug")
	bool IsMyFeatureEnabled() const;

	UPROPERTY(BlueprintReadOnly, Category = "LXR|Debug")
	TObjectPtr<UULXRDebugEntry> DebugEntryWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LXR|Debug")
	ELXRDebugFeature MyDebugFeature;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LXR|Debug")
	TSubclassOf<UActorComponent> GetMyDebugComponentClass() const;

	UFUNCTION(BlueprintCallable, Category = "LXR|Debug")
	void SetMyComponent(UActorComponent* InComponent);


	void PrepareWidget();
	void SetDebugEntryWidget(UULXRDebugEntry& InEntryWidget);
	void DebugFeaturesChanged();
	void NativeCustomTick();


	TSharedPtr<FLXRDebugFeatures> GetMyDebugFeatures() const;
};
