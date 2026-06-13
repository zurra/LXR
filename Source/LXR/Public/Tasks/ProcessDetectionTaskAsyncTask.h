// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "LXRDetectionComponent.h"
#include "Async/AsyncWork.h"


class FProcessDetectionTaskAsyncTask : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FProcessDetectionTaskAsyncTask>;

public:
	FProcessDetectionTaskAsyncTask(TSharedPtr<FDetectionTaskData> InDetectionTaskData)
		: Data(InDetectionTaskData) 
	{	}

protected:
	TSharedPtr<FDetectionTaskData> Data;

	FORCEINLINE const UWorld* GetWorld() const
	{
		return Data->DetectionComponent->GetWorld();
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FProcessDetectionTaskAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}

	void DoWork();
	// void DoWork_New();
};
