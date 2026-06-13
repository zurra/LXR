// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "FTraceTaskParams.h"
#include "FSilhouetteTraceTaskParams.generated.h"

USTRUCT(BlueprintType)
struct FSilhouetteTraceTaskParams : public FTraceTaskParams
{
	GENERATED_BODY()

public:
	FSilhouetteTraceTaskParams() : FSilhouetteTraceTaskParams(DefaultQuality)
	{
	}

	FSilhouetteTraceTaskParams(ECheckQuality Quality) : FTraceTaskParams()
	{
		FSilhouetteTraceTaskParams::SetQuality(Quality);
	}

	//Amount of segments in a cone.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Silhouette")
	int SilhouetteEdges;

	//Distance of Silhouette Check
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Silhouette")
	int SilhouetteDistance;

	//Distance per trace group in silhouette trace.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Silhouette")
	int DistancePerSegment;


	virtual void SetQuality(ECheckQuality Quality) override
	{
		SilhouetteDistance = 2000;
		SilhouetteEdges = 6;
		DistancePerSegment = 300;

		switch (Quality)
		{
		case ECheckQuality::Low:
			SilhouetteDistance *= 0.75;
			SilhouetteEdges *= 0.75;
			DistancePerSegment *= 1.5;

			break;
		case ECheckQuality::Medium:
			SilhouetteDistance *= 1;
			SilhouetteEdges *= 1;
			DistancePerSegment *= 1;

			break;
		case ECheckQuality::High:
			SilhouetteDistance *= 1.5;
			SilhouetteEdges *= 1.25;
			DistancePerSegment *= 0.75;
			break;
		case ECheckQuality::Epic:
			SilhouetteDistance *= 2;
			SilhouetteEdges *= 1.5;
			DistancePerSegment *= 0.5;
			break;
		case ECheckQuality::Custom:
			break;
		default: ;
		}
	}
};

