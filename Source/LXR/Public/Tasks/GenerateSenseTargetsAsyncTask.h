// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "LXRLightSenseComponent.h"
#include "Async/AsyncWork.h"


class FGenerateSenseTargetsAsyncTask : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FGenerateSenseTargetsAsyncTask>;

public:
	FGenerateSenseTargetsAsyncTask(TSharedPtr<FSenseTaskData> InSenseTaskData)
		: Data(InSenseTaskData)
	{
	}

protected:

	TSharedPtr<FSenseTaskData> Data;
	
	FORCEINLINE const UWorld* GetWorld() const
	{
		return Data->SenseComponent->GetWorld();
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FGenerateSenseTargetsAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}

	void DoWork();
	void GenerateSenseTargets();
	void FilterGeneratedTargets();
};
