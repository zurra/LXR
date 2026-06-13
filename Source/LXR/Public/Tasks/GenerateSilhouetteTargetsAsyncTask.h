// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "LXR_Shared.h"
#include "Async/AsyncWork.h"
#include "GameFramework/Actor.h"


struct FSilhouetteTraceTaskParams;
struct FSilhouetteTaskData;

class FGenerateSilhouetteTargetsAsyncTask : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FGenerateSilhouetteTargetsAsyncTask>;

public:
	FGenerateSilhouetteTargetsAsyncTask(FSilhouetteTaskData& InSilhouetteTaskData, FSilhouetteTraceTaskParams& InSilhouetteTraceTaskParams)
		: TaskData(&InSilhouetteTaskData), TraceTaskData(&InSilhouetteTraceTaskParams)
	{
	}

protected:

	FSilhouetteTaskData* TaskData;
	FSilhouetteTraceTaskParams* TraceTaskData;

	
	const UWorld* GetWorld() const;
	
	TStatId GetStatId() const;

	void DoWork();
	void GenerateSilhouetteTargets();
	void FilterGeneratedTargets();
};
