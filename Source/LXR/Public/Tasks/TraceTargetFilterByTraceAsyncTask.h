// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "Async/AsyncWork.h"
#include "BlueprintAsyncActions/FilterTraceTargetsByTrace.h"


class FTraceTargetFilterByTraceAsyncTask : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FTraceTargetFilterByTraceAsyncTask>;

public:
	FTraceTargetFilterByTraceAsyncTask(
		UFilterTraceTargetsByTrace* InTaskObject,
		TArray<AActor*> InActorsToIgnore,
		FVector InStartLocation,
		TSet<FVector> InTraceTargets,
		ECollisionChannel InTraceChannel)
		: TaskObject(InTaskObject),
		  ActorsToIgnore(InActorsToIgnore),
		  StartLocation(InStartLocation),
		  TraceTargets(InTraceTargets),
		  TraceChannel(InTraceChannel)
	{
	}

protected:
	UFilterTraceTargetsByTrace* TaskObject;
	TArray<AActor*> ActorsToIgnore;
	FVector StartLocation;
	TSet<FVector> TraceTargets;
	ECollisionChannel TraceChannel;
	FCollisionQueryParams Params;

	FORCEINLINE const UWorld* GetWorld() const
	{
		return TaskObject->GetWorld();
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLightSenseTraceTargetAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}

	void DoWork();
};
