// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "FTraceTaskParams.h"
#include "FConeTraceTaskParams.generated.h"


USTRUCT(BlueprintType)
struct FConeTraceTaskParams : public FTraceTaskParams
{
	GENERATED_BODY()

public:
	// Default constructor - delegated to the parameterized constructor
	FConeTraceTaskParams() : FConeTraceTaskParams(DefaultQuality)
	{
	} // Call parameterized constructor with default quality

	FConeTraceTaskParams(ECheckQuality Quality) : FTraceTaskParams() // Explicitly call the base class constructor
	{
		FConeTraceTaskParams::SetQuality(Quality); // Set quality during construction
	}

	//Amount of segments in a cone.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Sense", meta=(HideEditConditionToggle, ClampMin = "1", ClampMax = "10", UIMin = "1", UIMax = "10"), AdvancedDisplay)
	int ConeTraces = 6;

	//Angle of the sense cone.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Sense")
	int ConeAngle = 20;

	//Distance per trace group in cone trace.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Sense", AdvancedDisplay)
	int DistancePerSegment = 250;

	//Cone distance.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Sense", AdvancedDisplay)
	int SenseDistance = 1500;

	//Min distance to other trace targets to approve new trace target.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Sense", AdvancedDisplay)
	int MinDistancePerTarget = 50;

	virtual void SetQuality(ECheckQuality Quality) override
	{
		switch (Quality)
		{
		case ECheckQuality::Low:
			this->ConeTraces = 4;
			this->ConeAngle = 20;
			this->DistancePerSegment = 300;
			this->SenseDistance = 1250;
			this->MinDistancePerTarget = 100;
			break;
		case ECheckQuality::Medium:
			this->ConeTraces = 6;
			this->ConeAngle = 25;
			this->DistancePerSegment = 250;
			this->SenseDistance = 1500;
			this->MinDistancePerTarget = 75;
			break;
		case ECheckQuality::High:
			this->ConeTraces = 8;
			this->ConeAngle = 25;
			this->DistancePerSegment = 200;
			this->SenseDistance = 1750;
			this->MinDistancePerTarget = 50;
		case ECheckQuality::Epic:
			this->ConeTraces = 10;
			this->ConeAngle = 30;
			this->DistancePerSegment = 150;
			this->SenseDistance = 2000;
			this->MinDistancePerTarget = 35;
			break;
		default: ;
		}
	}
};
