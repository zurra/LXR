// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "Tasks/TraceTargetFilterByTraceAsyncTask.h"
#include "LXRSubsystem.h"
#include "Engine/World.h"
#if DEBUG_RENDERING
#include "DrawDebugHelpers.h"
#endif


void FTraceTargetFilterByTraceAsyncTask::DoWork()
{

	Params = FCollisionQueryParams("FTraceTargetFilterByTraceAsyncTask", SCENE_QUERY_STAT_ONLY(KismetTraceUtils), false);
	Params.AddIgnoredActors(ActorsToIgnore);

	TSet<FVector> OutTargets;
	for (const FVector& End : TraceTargets)
	{
		if (!GetWorld()->LineTraceTestByChannel(StartLocation, End, TraceChannel, Params))
		{
			OutTargets.Add(End);
		}
	}
	TaskObject->TaskDone(OutTargets);
}
