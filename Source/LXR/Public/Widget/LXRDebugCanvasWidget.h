// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.


#pragma once

#include "CoreMinimal.h"
#include "ULXRDebugEntry.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Data/LXRDebugEntryObject.h"
#include "LXRDebugCanvasWidget.generated.h"

class UListView;
class UVerticalBox;

UCLASS()
class LXR_API UULXRDebugCanvasWidget : public UUserWidget
{
	GENERATED_BODY()

public:


	UULXRDebugCanvasWidget(const FObjectInitializer& OI);
	
	void AddDebugObject(ULXRDebugEntryObject* DebugEntryObject, bool IsLocalPawn) const;
	void RemoveDebugObject(AActor* InActor, bool IsLocalPawn) const;

	
	UPROPERTY(EditAnywhere, meta = (BindWidget),Category="LXR|Debug")
	TObjectPtr<UListView> DebugListContainer;

	UPROPERTY(EditAnywhere,meta = (BindWidget),Category="LXR|Debug")
	TObjectPtr<UListView> LocalPlayerDebugListView;
	
	
};
