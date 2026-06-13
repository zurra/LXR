// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.


#include "LXRMethodObject.h"
#include "LXRSourceComponent.h"
#include "LXRLightSenseComponent.h"
#include "LXRSilhouetteDetectionComponent.h"

AActor* ULXRMethodObject::GetOwner() const
{
	if (IsValid(Owner))
		return Owner;
	return nullptr;
}

ULXRSourceComponent* ULXRMethodObject::GetSourceComponent() const
{
	if (IsValid(SourceComponent))
	{
		return SourceComponent;
	}
	return nullptr;
}

ULXRLightSenseComponent* ULXRMethodObject::GetLightSenseComponent() const
{
	if (IsValid(SenseComponent))
	{
		return SenseComponent;
	}
	return nullptr;
}

ULXRSilhouetteDetectionComponent* ULXRMethodObject::GetSilhouetteComponent() const
{
	if (IsValid(SilhouetteDetectionComponent))
	{
		return SilhouetteDetectionComponent;
	}
	return nullptr;
}

UWorld* ULXRMethodObject::GetWorld() const
{
	if (IsValid(Owner))
		return Owner->GetWorld();

	if (!HasAnyFlags(RF_ClassDefaultObject) && !GetOuter()->HasAnyFlags(RF_BeginDestroyed) && !GetOuter()->IsUnreachable())
	{
		//Try to get the world from the owning actor if we have one
		const AActor* Outer = GetTypedOuter<AActor>();
		if (Outer != nullptr)
		{
			return Outer->GetWorld();
		}
	}

	//Else return null - the latent action will fail to initialize
	return nullptr;
}
