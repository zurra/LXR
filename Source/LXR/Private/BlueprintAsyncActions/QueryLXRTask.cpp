// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#include "BlueprintAsyncActions/QueryLXRTask.h"
#include "Async/TaskGraphInterfaces.h"
#include "Async/Async.h"
#include "Tasks/QueryLXRAsyncTask.h"

UQueryLXRTask* UQueryLXRTask::QueryLocationsLXR_Async(const AActor* Querier, const TArray<FVector>& Points, TArray<ULXRSourceComponent*> InLights)
{
	UQueryLXRTask* BlueprintNode = NewObject<UQueryLXRTask>();
	if (IsValid(Querier))
		BlueprintNode->WorldContextObject = const_cast<AActor*>(Querier);
	
	BlueprintNode->Points = Points;
	BlueprintNode->Lights = InLights;
	return BlueprintNode;
}

void UQueryLXRTask::TaskDone(const FTargetLXRData& InIlluminatedTargets, const TSet<FLightSourcePassedData>& InPassedData)
{
	IlluminatedTargets = InIlluminatedTargets;
	PassedData = InPassedData;
	TWeakObjectPtr<UQueryLXRTask> WeakSelf(this);
	FTargetLXRData CapturedTargets = IlluminatedTargets;
	TSet<FLightSourcePassedData> CapturedPassedData = PassedData;
	AsyncTask(ENamedThreads::GameThread, [WeakSelf, CapturedTargets, CapturedPassedData]()
	{
		if (UQueryLXRTask* Self = WeakSelf.Get())
			Self->OnQueryLXRDone.Broadcast(CapturedTargets, CapturedPassedData);
	});
}


void UQueryLXRTask::Activate()
{
	(new FAutoDeleteAsyncTask<FQueryLXRAsyncTask>(this, WorldContextObject, Points, Lights))->StartBackgroundTask();
}
