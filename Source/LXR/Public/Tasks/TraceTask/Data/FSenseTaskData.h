// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "FTraceTaskData.h"

struct FLightSourcePassedData;

struct LXR_API FSenseTaskData : public FTraceTaskData
{
	FSenseTaskData(): SenseComponent(nullptr), MemoryComponent(nullptr), SenseLocation(), SenseRotation(), LightMemoryEnabled(false)
	{
	};

	FSenseTaskData(const FSenseTaskData& Other)
	: FTraceTaskData(Other) 
	, SenseComponent(Other.SenseComponent)
	, MemoryComponent(Other.MemoryComponent)
	, SenseLocation(Other.SenseLocation)
	, SenseRotation(Other.SenseRotation)
	, LightMemoryEnabled(Other.LightMemoryEnabled)
	, SourcesSensedThisTick(Other.SourcesSensedThisTick)
	, SensedLightPassedData(Other.SensedLightPassedData)
	, SensedLightsToRemove(Other.SensedLightsToRemove)
	{
	}


	ULXRLightSenseComponent* SenseComponent;
	ULXRMemoryComponent* MemoryComponent;

	FVector SenseLocation;
	FRotator SenseRotation;
	bool LightMemoryEnabled;

	TMap<TWeakObjectPtr<ULXRSourceComponent>, int> SourcesSensedThisTick;
	TSet<FLightSourcePassedData> SensedLightPassedData;
	TArray<TWeakObjectPtr<ULXRSourceComponent>> SensedLightsToRemove;

	void DecreaseSourcesRefTimer();
	void SetSourceAsSensed(const FLightSourcePassedData* InData);

	bool IsSourceSensedThisTick(const TWeakObjectPtr<ULXRSourceComponent> SourceComponent);
	bool IsValid() const;

	bool operator==(const FSenseTaskData& Other) const;
	bool operator==(const ULXRLightSenseComponent& InSenseComponent) const;
};
