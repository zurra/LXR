// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "Async/AsyncWork.h"
#include "BlueprintAsyncActions/QueryLXRTask.h"


class FQueryLXRAsyncTask : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FQueryLXRAsyncTask>;

public:
	FQueryLXRAsyncTask(UQueryLXRTask* InTaskObject,const UObject* InQuerier, const TArray<FVector>& InTraceLocations, const TArray<ULXRSourceComponent*> InLights) 
	{
		TaskObject = InTaskObject;
		Querier = InQuerier;
		TraceLocations = InTraceLocations;
		Lights = InLights;
	}

protected:

	UQueryLXRTask* TaskObject;
	const UObject* Querier;
	TArray<FVector> TraceLocations;
	TArray<ULXRSourceComponent*> Lights;
	
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FQueryLXRAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}
	
void DoWork() const;
};
