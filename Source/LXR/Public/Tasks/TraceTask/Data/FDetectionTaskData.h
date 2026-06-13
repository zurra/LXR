// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.
#pragma once
#include "FTraceTaskData.h"
#include "FDetectionTaskData.generated.h"


USTRUCT(BlueprintType)
struct LXR_API FDetectionTaskData : public FTraceTaskData
{
	GENERATED_BODY()
	FDetectionTaskData(): DetectionComponent(nullptr)
	{
	}

	UPROPERTY()
	TObjectPtr<ULXRDetectionComponent> DetectionComponent;

	TMap<int, int> SourcesDetectedThisTick;
	
	// TArray<int32> Buckets[3];
	// TArray<int32>& RelevantLights()         { return Buckets[0]; }
	// TArray<int32>& RelevantLightsToRemove() { return Buckets[1]; }
	// TArray<int32>& RelevantLightsToAdd()    { return Buckets[2]; }
	
	int RelevantLightBatchCount = 50;
	int RelevantLightIndex = -1;

	bool IsLightMemoryEnabled = false;

	// void CheckAndRemoveNonRelevantLights();
	// void CheckAndAddNewRelevantLights();

	void RemovePassedLight(FLightSourcePassedData& PassedDat);
	void AddPassedLight(FLightSourcePassedData& PassedData);

	// void AddRelevantLight(int InEntryIndex);
	// void RemoveRelevantLight(int EntryIndex);
	

	// void GetNextRelevantCheckLightBatch(TArray<TWeakObjectPtr<ULXRSourceComponent>>& OutLightBatch);
	void DecreaseSourcesRefTimer();

	bool IsSourceStillRelevant(int EntryIndex);

	void SetSourceAsDetected(int EntryIndex);

	bool operator==(const FDetectionTaskData& Other) const;

	bool operator==(const ULXRDetectionComponent& InDetectionComponent) const;

	bool IsValid() const;
};
