// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "FTraceTaskData.h"
#include "Tasks/TraceTask/Params/FSilhouetteTraceTaskParams.h"
#include "FSilhouetteTaskData.generated.h"

USTRUCT(BlueprintType)
struct LXR_API FSilhouetteTaskData : public FTraceTaskData
{
	GENERATED_BODY()

	FSilhouetteTaskData(): DetectionComponent(nullptr), Instigator(nullptr), bDrawDebug(false), TraceTargetBatchCount(0), TargetsRequired(0), Bounds(), InstigatorViewPointLocation(), InstigatorViewPointRotation(),
	                       SilhouetteLocation(),
	                       SilhouetteRotation(),
	                       SilhouetteCalculationType()
	{
	};

	UPROPERTY()
	TObjectPtr<ULXRDetectionComponent> DetectionComponent;
	UPROPERTY()
	TObjectPtr<AActor> Instigator;

	bool bDrawDebug;

	int TraceTargetBatchCount;
	int TargetsRequired;

	FBoxSphereBounds Bounds;

	FVector InstigatorViewPointLocation;
	FRotator InstigatorViewPointRotation;
	FVector SilhouetteLocation;
	FRotator SilhouetteRotation;

	FSilhouetteTraceTaskParams SilhouetteTraceTaskParams;

	ETaskProcessType SilhouetteCalculationType;

	bool operator==(const FSilhouetteTaskData& Other) const;

	bool operator==(const ULXRDetectionComponent& InDetectionComponent) const;
};
