// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.


#include "Widget/ULXRDebugFeature.h"
#include "Widget/ULXRDebugFeatureContainer.h"


void UULXRDebugFeatureContainer::DebugFeaturesChanged()
{
	DirectDebug->DebugFeaturesChanged();
	SenseDebug->DebugFeaturesChanged();
	FluxDebug->DebugFeaturesChanged();


	SetVisibility(DirectDebug->IsVisible() || SenseDebug->IsVisible() || FluxDebug->IsVisible() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UULXRDebugFeatureContainer::TickWidgets() const
{
	DirectDebug->NativeCustomTick();
	SenseDebug->NativeCustomTick();
	FluxDebug->NativeCustomTick();
}

void UULXRDebugFeatureContainer::PrepareWidgets() const
{
	DirectDebug->PrepareWidget();
	SenseDebug->PrepareWidget();
	FluxDebug->PrepareWidget();
}

void UULXRDebugFeatureContainer::SetDebugEntryWidget(UULXRDebugEntry& InEntryWidget)
{
	DirectDebug->SetDebugEntryWidget(InEntryWidget);
	SenseDebug->SetDebugEntryWidget(InEntryWidget);
	FluxDebug->SetDebugEntryWidget(InEntryWidget);
}
