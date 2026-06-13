// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#include "BlueprintAsyncActions/FilterTraceTargetsByTrace.h"
#include "Tasks/QueryLXRAsyncTask.h"
#include "Tasks/TraceTargetFilterByTraceAsyncTask.h"
#include "Async/TaskGraphInterfaces.h"
#include "Async/Async.h"

UFilterTraceTargetsByTrace* UFilterTraceTargetsByTrace::FilterTraceTargetsByTrace_Async(const UObject* WorldContextObject,const FVector& FilterTraceStartLocation, const TSet<FVector>& TraceTargets, TArray<AActor*> ActorsToIgnore, ECollisionChannel TraceChannel)
{


	UFilterTraceTargetsByTrace* BlueprintNode = NewObject<UFilterTraceTargetsByTrace>();
	BlueprintNode->WorldContextObject = const_cast<UObject*>(WorldContextObject);
	BlueprintNode->StartLocation = FilterTraceStartLocation;
	BlueprintNode->TraceTargets = TraceTargets;
	BlueprintNode->ActorsToIgnore = ActorsToIgnore;
	return BlueprintNode;
}

void UFilterTraceTargetsByTrace::TaskDone(const TSet<FVector>& Targets)
{
	FilteredTargets = Targets;
	AsyncTask(ENamedThreads::GameThread, [&]()
	{
		OnTraceTargetOperationDone.Broadcast(FilteredTargets);
	});
}


void UFilterTraceTargetsByTrace::Activate()
{
	(new FAutoDeleteAsyncTask<FTraceTargetFilterByTraceAsyncTask>(this,  ActorsToIgnore, StartLocation, TraceTargets, TraceChannel))->StartBackgroundTask();
	
}

UWorld* UFilterTraceTargetsByTrace::GetWorld() const
{
	return WorldContextObject->GetWorld();
}
