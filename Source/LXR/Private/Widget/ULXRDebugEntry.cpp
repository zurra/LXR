// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#include "Widget/ULXRDebugEntry.h"
#include "LXRDebugSubsystem.h"
#include "LXRDetectionComponent.h"
#include "Components/TextBlock.h"
#include "Widget/ULXRDebugFeature.h"
#include "Widget/ULXRDebugFeatureContainer.h"
#include "Widget/Data/LXRDebugEntryObject.h"


void UULXRDebugEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	MyEntryObject = Cast<ULXRDebugEntryObject>(ListItemObject);

	WidgetTickDelegate.BindUObject(this, &UULXRDebugEntry::TickWidgets);
	GetWorld()->GetTimerManager().SetTimer(WidgetTickTimerHandle, WidgetTickDelegate, 0.05f, true);
	if (MyEntryObject != nullptr)
	{
		MyEntryObject.Get()->MyDebugFeatures->OnDebugFeaturesChanged.BindUObject(this, &UULXRDebugEntry::DebugFeaturesChanged);
		PrepareWidgets();
		EntryReady();

		if (TObjectPtr<UActorComponent> ActorDetectionComponent = MyEntryObject.Get()->GetMyComponent(ELXRDebugFeature::Direct))
		{
			ULXRDetectionComponent* DetectionComponent = Cast<ULXRDetectionComponent>(ActorDetectionComponent);
			DetectionComponent->OnSeenBySilhouette.AddDynamic(this, &UULXRDebugEntry::SeenBySilhouette);

			if (IsValid(BorderSeenBySilhouetteLeft) && IsValid(BorderSeenBySilhouetteRight))
			{
				BorderSeenBySilhouetteLeft->SetVisibility(ESlateVisibility::Hidden);
				BorderSeenBySilhouetteRight->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

void UULXRDebugEntry::PrepareWidgets()
{
	SocketDebug->SetDebugEntryWidget(*this);
	DetectionDebugs->SetDebugEntryWidget(*this);
	CaptureDebug->SetDebugEntryWidget(*this);

	SocketDebug->PrepareWidget();
	DetectionDebugs->PrepareWidgets();
	CaptureDebug->PrepareWidget();

	txtActorName.Get()->SetText(FText::FromString(GetMyDebugActor()->GetActorNameOrLabel()));
}

void UULXRDebugEntry::TickWidgets()
{
	SocketDebug->NativeCustomTick();
	DetectionDebugs->TickWidgets();
	CaptureDebug->NativeCustomTick();

	CustomTick();
}


void UULXRDebugEntry::DebugFeaturesChanged() const
{
	SocketDebug->DebugFeaturesChanged();
	DetectionDebugs->DebugFeaturesChanged();
	CaptureDebug->DebugFeaturesChanged();
}

AActor* UULXRDebugEntry::GetMyDebugActor() const
{
	return MyEntryObject->GetMyDebugActor();
}

FLinearColor UULXRDebugEntry::GetMyDebugColor()
{
	return MyEntryObject->DebugColor;
}

void UULXRDebugEntry::SeenBySilhouette(AActor* Actor)
{
	static FTimerHandle TimerHandle;

	if (IsValid(BorderSeenBySilhouetteLeft) && IsValid(BorderSeenBySilhouetteRight))
	{
		BorderSeenBySilhouetteLeft->SetVisibility(ESlateVisibility::Visible);
		BorderSeenBySilhouetteRight->SetVisibility(ESlateVisibility::Visible);
	}

	if (!GetWorld()->GetTimerManager().TimerExists(TimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda([&]()
		{
			if (IsValid(BorderSeenBySilhouetteLeft) && IsValid(BorderSeenBySilhouetteRight))
			{
				BorderSeenBySilhouetteLeft->SetVisibility(ESlateVisibility::Hidden);
				BorderSeenBySilhouetteRight->SetVisibility(ESlateVisibility::Hidden);
			}
		}), 3.f, false);
	}
}

void UULXRDebugEntry::NativeDestruct()
{
	GetWorld()->GetTimerManager().ClearTimer(WidgetTickTimerHandle);
	WidgetTickDelegate.Unbind();
	Super::NativeDestruct();
}
