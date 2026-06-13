// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "Data/LXRDebugEntryObject.h"
#include "Engine/TimerHandle.h"
#include "TimerManager.h"
#include "ULXRDebugEntry.generated.h"


class UBorder;
class UULXRDebugFeature;
class UULXRDebugFeatureContainer;
class UVerticalBox;
class UTextBlock;
/**
 * 
 */
UCLASS()
class LXR_API UULXRDebugEntry : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	/** Initialize data to widget from Entry Data Object*/
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	// virtual void NativeOnEntryReleased() override;

	virtual void NativeDestruct() override;

	void PrepareWidgets();
	void TickWidgets();
	void DebugFeaturesChanged() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "LXR|Debug")
	void CustomTick();

	UFUNCTION(BlueprintImplementableEvent, Category = "LXR|Debug")
	void EntryReady();
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LXR|Debug")
	AActor* GetMyDebugActor() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LXR|Debug")
	FLinearColor GetMyDebugColor();

	UPROPERTY(meta = (BindWidget), EditAnywhere, Category = "LXR|Debug")
	TObjectPtr<UTextBlock> txtActorName;

	UPROPERTY(meta = (BindWidget),BlueprintReadOnly, EditAnywhere, Category = "LXR|Debug")
	TObjectPtr<UBorder> BorderSeenBySilhouetteLeft;
	
	UPROPERTY(meta = (BindWidget),BlueprintReadOnly, EditAnywhere, Category = "LXR|Debug")
	TObjectPtr<UBorder> BorderSeenBySilhouetteRight;

	UPROPERTY(meta = (BindWidget),BlueprintReadOnly, EditAnywhere, Category = "LXR|Debug")
	TObjectPtr<UULXRDebugFeature> SocketDebug;

	UPROPERTY(meta = (BindWidget),BlueprintReadOnly, EditAnywhere, Category = "LXR|Debug")
	TObjectPtr<UULXRDebugFeatureContainer> DetectionDebugs;

	UPROPERTY(meta = (BindWidget),BlueprintReadOnly, EditAnywhere, Category = "LXR|Debug")
	TObjectPtr<UULXRDebugFeature> CaptureDebug;
	
	UPROPERTY()
	TObjectPtr<ULXRDebugEntryObject> MyEntryObject;

	UFUNCTION()
	void SeenBySilhouette(AActor* Actor);
	
	FTimerHandle WidgetTickTimerHandle;
	FTimerDelegate WidgetTickDelegate;
	
};
