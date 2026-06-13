// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "LXRLightSenseComponent.h"
#include "Async/AsyncWork.h"


class FProcessSenseTaskAsyncTask : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FProcessSenseTaskAsyncTask>;

public:
	FProcessSenseTaskAsyncTask(TSharedPtr<FSenseTaskData> InSenseTaskData)
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
		RETURN_QUICK_DECLARE_CYCLE_STAT(FProcessSenseTaskAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}

	void DoWork();

};
