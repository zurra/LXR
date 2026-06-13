// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "LXROctreeVolume.generated.h"

/**
 * 
 */
UCLASS()
class LXR_API ALXROctreeVolume : public AVolume
{
	GENERATED_BODY()

	ALXROctreeVolume();

	virtual void PostActorCreated() override;
};
