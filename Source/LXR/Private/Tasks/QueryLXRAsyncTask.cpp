// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#include "Tasks/QueryLXRAsyncTask.h"

#include "LXRFunctionLibrary.h"
#include "LXRSubsystem.h"

void FQueryLXRAsyncTask::DoWork() const
{
	TSet<FLightSourcePassedData> PassedData;
	FTargetLXRData IlluminatedTargets;
	ULXRFunctionLibrary::QueryLocationsLXR(Querier, PassedData, IlluminatedTargets, Lights, TraceLocations, {});
	TaskObject->TaskDone(IlluminatedTargets,PassedData);
}
