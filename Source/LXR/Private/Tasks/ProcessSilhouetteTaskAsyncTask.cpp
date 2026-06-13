// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "Tasks/ProcessSilhouetteTaskAsyncTask.h"
#include "LXRFunctionLibrary.h"
#include "LXRSourceComponent.h"
#include "LXR_Shared.h"
#include "Engine/World.h"
#include "Tasks/TraceTask/Data/FSilhouetteTaskData.h"
#if DEBUG_RENDERING
#include "DrawDebugHelpers.h"
#endif


const UWorld* FProcessSilhouetteTaskAsyncTask::GetWorld() const
{
	return Data->Instigator->GetWorld();
}

TStatId FProcessSilhouetteTaskAsyncTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FLightSenseTraceTargetAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
}

void FProcessSilhouetteTaskAsyncTask::DoWork()
{
	Data->StartState();

	while (Data->TargetIterator > INDEX_NONE)
	{
		if (Data->Instigator->GetWorld()->bIsTearingDown) break;

		auto TracePoints = Data->GetNextTraceTargets(Data->TraceTargetBatchCount);
		if (TracePoints.IsEmpty()) break;
		for (const auto& Light : Data->Lights)
		{
			if (!Light.IsValid()) continue;
			FLightSourcePassedData* PassedDataPtr = Data->PassedDatas.Find(Light);
			if (PassedDataPtr == nullptr)
			{
				auto ElementID = Data->PassedDatas.Add(Light);
				PassedDataPtr = &Data->PassedDatas[ElementID];

			}
			
			Data->RelevancyRequiredAmountToPass = Data->TargetsRequired < 1 ? TracePoints.Num() * Data->TargetsRequired : Data->TargetsRequired;
			Data->RelevantRequiredAmountToPass = Data->RelevancyRequiredAmountToPass;

			ULXRFunctionLibrary::CheckIsLightRelevant(Data->Instigator, *Data, *PassedDataPtr, {});
			ULXRFunctionLibrary::CheckVisibility(Data->Instigator, *Data, *PassedDataPtr, {});
		}
	}
	Data->NextState(); // analyze
}
