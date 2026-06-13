// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.


#include "LXROctreeVolume.h"
#include "EngineUtils.h"
#include "UnrealEngine.h"
#include "Components/BrushComponent.h"

ALXROctreeVolume::ALXROctreeVolume()
{
	static FName CollisionProfileName(TEXT("NoCollision"));
	GetBrushComponent()->SetCollisionProfileName(CollisionProfileName);
	GetBrushComponent()->SetGenerateOverlapEvents(false);
}

void ALXROctreeVolume::PostActorCreated()
{
	Super::PostActorCreated();
	const TActorIterator<ALXROctreeVolume> It(GetWorld());
	if (It)
	{
		if (*It != this)
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString("LXROctreeVolume already in level \n Make sure there is only one LXROctreeVolume!"));
	}
}
