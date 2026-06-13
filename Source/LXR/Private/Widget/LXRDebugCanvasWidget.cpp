// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.


#include "Widget/LXRDebugCanvasWidget.h"

#include "LXR.h"
#include "Components/ListView.h"
#include "Kismet/GameplayStatics.h"
#include "Widget/ULXRDebugFeatureContainer.h"
#include "Widget/Data/LXRDebugEntryObject.h"


UULXRDebugCanvasWidget::UULXRDebugCanvasWidget(const FObjectInitializer& OI) : Super(OI)
{
}

void UULXRDebugCanvasWidget::AddDebugObject(ULXRDebugEntryObject* DebugEntryObject, bool IsLocalPawn) const
{
	if (IsLocalPawn)
	{
		if (LocalPlayerDebugListView->GetNumItems() == 0)
		{
			LocalPlayerDebugListView->AddItem(DebugEntryObject);
		}
		else
		{
			UE_LOG(LogLXR, Warning, TEXT("Local Debug Container already has content."));
		}
	}
	else
	{
		DebugListContainer.Get()->AddItem(DebugEntryObject);
		DebugListContainer->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UULXRDebugCanvasWidget::RemoveDebugObject(AActor* InActor, bool IsLocalPawn) const
{
	if (IsLocalPawn)
	{
		if (LocalPlayerDebugListView->GetNumItems() == 1)
		{
			auto Item = LocalPlayerDebugListView.Get()->GetItemAt(0);
			LocalPlayerDebugListView.Get()->RemoveItem(Item);
		}
	}
	else
	{
		TArray<UObject*> EntryObjects = DebugListContainer.Get()->GetListItems();
		ULXRDebugEntryObject* EntryObjectToRemove = nullptr;
		int Index = -1;
		for (int32 i = 0; i < EntryObjects.Num(); i++)
		{
			if (EntryObjectToRemove != nullptr) break;
			if (IsValid(EntryObjects[i]))
			{
				if (ULXRDebugEntryObject* EntryObject = Cast<ULXRDebugEntryObject>(EntryObjects[i]))
				{
					if (EntryObject->GetMyDebugActor() == InActor)
						EntryObjectToRemove = EntryObject;
				}
			}
		}
		if (EntryObjectToRemove != nullptr)
		{
			DebugListContainer->RemoveItem(EntryObjectToRemove);

			if (DebugListContainer->GetNumItems() == 0)
				DebugListContainer->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
