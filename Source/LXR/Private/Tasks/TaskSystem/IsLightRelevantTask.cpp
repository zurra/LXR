// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#include "Tasks/TaskSystem/IsLightRelevantTask.h"

#include "LXRSourceComponent.h"
#include "Tasks/TraceTask/Data/FTraceTaskData.h"

// bool IsLightRelevantTask::CheckIsLightRelevant_New(const AActor* Instigator, const FTraceTaskData& TraceTaskData, FLightSourcePassedData_New& PassedData, const FQueryLXRDebugOptions& QueryLXRDebugOptions)
// {
// 	if (!Instigator) return false;
// 	TArray<FVector> TraceTargets = TraceTaskData.ChosenTracePoints;
//
// 	if (!PassedData.LightSourceActor.IsValid()) return false;
//
// 	const TWeakObjectPtr<ULXRSourceComponent> LightSourceComponent = PassedData.LXRSourceComponent;
// 	TArray<ULightComponent*> LightComponents = LightSourceComponent->GetMyLightComponents();
//
// 	PassedData.Clear();
//
// 	int PassedChecks = 0;
// 	int ComponentIdx = 0;
// 	bool Passed = false;
// 	bool ForceCheckBecauseOfMemoryState = false;
// 	const float DebugDrawTime = QueryLXRDebugOptions.DebugRelevancyDrawTime;
// 	const float RequiredChecksToPassAmount = PassedData.RequiredChecksToPassAmount < 1 ? TraceTargets.Num() * PassedData.RequiredChecksToPassAmount : PassedData.RequiredChecksToPassAmount;
// }
