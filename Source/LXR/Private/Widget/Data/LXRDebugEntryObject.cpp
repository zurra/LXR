// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.


#include "Widget/Data/LXRDebugEntryObject.h"

#include "LXRDebugSubsystem.h"
#include "LXRDetectionComponent.h"
#include "LXRLightSenseComponent.h"

void ULXRDebugEntryObject::Initialize(AActor* InDebugActor, const FLXRDebugFeatures& InDebugFeatures)
{
	MyDebugFeatures = MakeShared<FLXRDebugFeatures>(InDebugFeatures);
	MyDebugFeatures->DebugActor = InDebugActor;
	MyDebugFeatures->OnDebugFeaturesChanged.BindWeakLambda(this, [this]()
	{
		if (DebugSettingsChanged.IsBound())
			DebugSettingsChanged.Execute();
	});

	ULXRDebugSubsystem* LXRDebugSubsystem = GetWorld()->GetSubsystem<ULXRDebugSubsystem>();
	if (LXRDebugSubsystem != nullptr)
	{
		DebugColor = LXRDebugSubsystem->GetDebugColor();
	}
}

AActor* ULXRDebugEntryObject::GetMyDebugActor() const
{
	return MyDebugFeatures.Get()->DebugActor;
}

void ULXRDebugEntryObject::SetComponent(ELXRDebugFeature InDebugFeature, UActorComponent* InComponent)
{
	switch (InDebugFeature)
	{
	case ELXRDebugFeature::None:
		break;
	case ELXRDebugFeature::Direct:
	case ELXRDebugFeature::Sockets:
		DetectionComponent = Cast<ULXRDetectionComponent>(InComponent);
		break;
	case ELXRDebugFeature::Sense:
		LightSenseComponent = Cast<ULXRLightSenseComponent>(InComponent);
		break;
	case ELXRDebugFeature::Capture:
		FluxLightDetectorComponent = Cast<ULXRFluxLightDetectorComponent>(InComponent);
		break;
	}
}

TObjectPtr<UActorComponent> ULXRDebugEntryObject::GetMyComponent(ELXRDebugFeature InDebugFeature)
{
	switch (InDebugFeature)
	{
	case ELXRDebugFeature::None:
		break;
	case ELXRDebugFeature::Direct:
	case ELXRDebugFeature::Sockets:
		return DetectionComponent;
	case ELXRDebugFeature::Sense:
		return LightSenseComponent;
	case ELXRDebugFeature::Capture:
		return FluxLightDetectorComponent;
	}
	return nullptr;
}
