// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "Tasks/ProcessDetectionTaskAsyncTask.h"

#include "LXRFunctionLibrary.h"
#include "Components/LightComponent.h"

#if DEBUG_RENDERING
#include "DrawDebugHelpers.h"
#endif


void FProcessDetectionTaskAsyncTask::DoWork()
{
	SCOPE_CYCLE_COUNTER(STAT_LightDetectionCheck);

	while (Data->TargetIterator > INDEX_NONE)
	{
		const TArray<FVector> TracePoints = Data->GetNextTraceTargets(Data->DetectionComponent->VisibilityLightBatchCount, true);

		const bool IsFromThread = Data->IsBackgroundThread;
		ULXRDetectionComponent* DetectionComponent = Data->DetectionComponent;
		Data->PassedDatas.Reset();
		for (int i = 0; i < Data->DetectionComponent->RelevantBucket.Num(); ++i)
		{
			int EntryIndex = Data->DetectionComponent->RelevantBucket[i];
			const TWeakObjectPtr SourceComponent = DetectionComponent->Entries[EntryIndex].Light.Get();
				FLightSourcePassedData* PassedDataPtr = Data->PassedDatas.Find(SourceComponent);
				if (PassedDataPtr == nullptr)
				{
					auto ElementID = Data->PassedDatas.Add(SourceComponent);
					PassedDataPtr = &Data->PassedDatas[ElementID];
				}
				Data->RelevancyRequiredAmountToPass = Data->DetectionComponent->TargetsRequired < 1 ? TracePoints.Num() * Data->DetectionComponent->TargetsRequired : Data->DetectionComponent->TargetsRequired;
				Data->RelevantRequiredAmountToPass = Data->DetectionComponent->TracesRequired < 1 ? TracePoints.Num() * Data->DetectionComponent->TracesRequired : Data->DetectionComponent->TracesRequired;

				Data->RelevancyRequiredAmountToPass = FMath::Max(1, Data->RelevancyRequiredAmountToPass);
				Data->RelevantRequiredAmountToPass = FMath::Max(1, Data->RelevantRequiredAmountToPass);

				if (!SourceComponent.IsValid()) continue;

				ULXRMemoryComponent* MemoryComponent = Data->DetectionComponent->MemoryComponent;
				const bool IsLightSourceEnabled = SourceComponent->IsEnabled();
				const bool UpdateMemory = Data->IsLightMemoryEnabled && SourceComponent->bIsMemorizable;

				if (IsLightSourceEnabled || UpdateMemory)
				{
					if (ULXRFunctionLibrary::CheckIsLightRelevant(Data->DetectionComponent->GetOwner(), *Data, *PassedDataPtr, Data->DetectionComponent->DebugOptions))
					{
						if (ULXRFunctionLibrary::CheckVisibility(Data->DetectionComponent->GetOwner(), *Data, *PassedDataPtr, Data->DetectionComponent->DebugOptions))
						{
							if (IsLightSourceEnabled)
								Data->AddPassedLight(*PassedDataPtr);

							if (Data->IsLightMemoryEnabled)
							{
								IsFromThread ? MemoryComponent->AddOrUpdateLightStateFromThread(SourceComponent) : MemoryComponent->AddOrUpdateLightState(SourceComponent);
							}
						}
					}
				}
			
		}
	}
	Data->TaskLock = false;
}

// void FProcessDetectionTaskAsyncTask::DoWork_New()
// {
// }
