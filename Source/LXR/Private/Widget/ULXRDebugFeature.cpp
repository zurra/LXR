// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.


#include "Widget/ULXRDebugFeature.h"

#include "LXRDetectionComponent.h"
#include "LXRFluxLightDetector.h"
#include "LXRLightSenseComponent.h"
#include "Components/PanelWidget.h"
#include "Widget/ULXRDebugEntry.h"

AActor* UULXRDebugFeature::GetMyDebugActor() const
{
    return DebugEntryWidget.Get()->GetMyDebugActor();
}

UActorComponent* UULXRDebugFeature::GetMyComponent(TSubclassOf<UActorComponent> FeatureComponent) const
{
    UActorComponent* ComponentToReturn;
    if (MyDebugFeature == ELXRDebugFeature::Sockets && FeatureComponent.Get() == ULXRFluxLightDetectorComponent::StaticClass())
    {
        ComponentToReturn = DebugEntryWidget->MyEntryObject->GetMyComponent(ELXRDebugFeature::Capture);
    }
    else
    {
        ComponentToReturn = DebugEntryWidget->MyEntryObject->GetMyComponent(MyDebugFeature);
    }
    return ComponentToReturn;
}

bool UULXRDebugFeature::IsMyFeatureEnabled() const
{
    auto DebugFeatures = GetMyDebugFeatures();

    bool DebugFeatureEnabled = false;
    bool IsComponentAvailable = false;
    bool FeatureEnabledInComponent = true;
    UActorComponent* MyFeatureComponent = nullptr;
    switch (MyDebugFeature)
    {
    case ELXRDebugFeature::Direct:
        DebugFeatureEnabled = DebugFeatures->bEnableDirect || DebugFeatures->bAutoDetect;
        MyFeatureComponent = DebugEntryWidget->MyEntryObject->GetMyComponent(ELXRDebugFeature::Direct);
        break;
    case ELXRDebugFeature::Sense:
        DebugFeatureEnabled = DebugFeatures->bEnableSense || DebugFeatures->bAutoDetect;
        MyFeatureComponent = DebugEntryWidget->MyEntryObject->GetMyComponent(ELXRDebugFeature::Sense);
        break;

    case ELXRDebugFeature::Capture:
        DebugFeatureEnabled = DebugFeatures->bEnableCapture || DebugFeatures->bAutoDetect;
        MyFeatureComponent = DebugEntryWidget->MyEntryObject->GetMyComponent(ELXRDebugFeature::Capture);
        break;
    case ELXRDebugFeature::Sockets:
        DebugFeatureEnabled = DebugFeatures->bEnableSockets || DebugFeatures->bAutoDetect;
        MyFeatureComponent = DebugEntryWidget->MyEntryObject->GetMyComponent(ELXRDebugFeature::Sockets);
        if (MyFeatureComponent)
        {
            if (auto DetectionComponent = Cast<ULXRDetectionComponent>(MyFeatureComponent))
            {
                FeatureEnabledInComponent = DetectionComponent->VisibilityTargetType == ETraceTarget::Sockets;
            }
        }
        break;

    case ELXRDebugFeature::None:
        return false;
    default: ;
    }
    IsComponentAvailable = MyFeatureComponent != nullptr;

    return DebugFeatureEnabled && IsComponentAvailable && FeatureEnabledInComponent;
}

TSubclassOf<UActorComponent> UULXRDebugFeature::GetMyDebugComponentClass() const
{
    switch (MyDebugFeature)
    {
    case ELXRDebugFeature::Direct:
    case ELXRDebugFeature::Sockets:
        return ULXRDetectionComponent::StaticClass();
    case ELXRDebugFeature::Sense:
        return ULXRLightSenseComponent::StaticClass();
    case ELXRDebugFeature::Capture:
        return ULXRFluxLightDetectorComponent::StaticClass();

    case ELXRDebugFeature::None:
        break;
    }
    return nullptr;
}


void UULXRDebugFeature::PrepareWidget()
{
    SetMyComponent(GetMyDebugActor()->GetComponentByClass(GetMyDebugComponentClass()));
    DebugFeaturesChanged();
    BP_WidgetReady();
}

void UULXRDebugFeature::DebugFeaturesChanged()
{
    SetVisibility(IsMyFeatureEnabled() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UULXRDebugFeature::NativeCustomTick()
{
    if (IsVisible())
        CustomTick();
}

void UULXRDebugFeature::SetMyComponent(UActorComponent* InComponent)
{
    DebugEntryWidget->MyEntryObject->SetComponent(MyDebugFeature, InComponent);
}

void UULXRDebugFeature::SetDebugEntryWidget(UULXRDebugEntry& InEntryWidget)
{
    DebugEntryWidget = &InEntryWidget;
}

TSharedPtr<FLXRDebugFeatures> UULXRDebugFeature::GetMyDebugFeatures() const
{
    return DebugEntryWidget->MyEntryObject->MyDebugFeatures;
}
