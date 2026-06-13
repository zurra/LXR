// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "CoreMinimal.h"
#include "LXR_Shared.h"
#include "Engine/EngineTypes.h"
#include "FTraceTaskData.generated.h"

class ULXRMemoryComponent;
class ULXRLightSenseComponent;
class ULXRDetectionComponent;

enum class ETaskState : uint8
{
	Cleanup,
	GeneratingTargets,
	Processing,
	Analyze,
	Max
};
enum class ETaskStatus : uint8
{
	Waiting,
	Started,
	Finished,
};


USTRUCT(BlueprintType)
struct LXR_API FTraceTaskData
{
public:
	GENERATED_BODY()

	FTraceTaskData(): TargetIterator(-1), bValidTick(false), IsBackgroundThread(false), RelevancyRequiredAmountToPass(0), RelevantRequiredAmountToPass(0), TraceType(), TraceChannel(),
	                  TaskState(ETaskState::Cleanup), TaskStatus(ETaskStatus::Waiting)
	{
	};
	FTraceTaskData(const TArray<FVector>& InTracePoints);

	void Reset(const TArray<FVector>& InTracePoints);

	ETaskState GetTaskState() const;
	ETaskStatus GetTaskStatus() const;
	void NextState();
	void SetState(ETaskState NewState);
	void StartState();
	void FinishState();
	bool IsStateStarted() const {return TaskStatus > ETaskStatus::Waiting;}

	void SetNewTracePointsAndResetIterator(const TArray<FVector>& InTracePoints);
	void SetLights(const TArray<TWeakObjectPtr<ULXRSourceComponent>>& InLights);
	TArray<FVector>& GetNextTraceTargets(const int& TargetBatchCount, bool All = false);

	TSet<FLightSourcePassedData> PassedDatas;

	TArray<TWeakObjectPtr<ULXRSourceComponent>> Lights;
	TArray<FVector> TracePoints;
	TArray<FVector> ChosenTracePoints;
	TWeakObjectPtr<AActor> Instigator;

	FTargetLXRData TargetLXRData;
	int TargetIterator;

	bool bValidTick;
	bool IsBackgroundThread;
	bool TaskLock = false;

	int RelevancyRequiredAmountToPass;
	int RelevantRequiredAmountToPass;
	EDetectionType TraceType;
	ECollisionChannel TraceChannel = ECC_Visibility;

private:
	ETaskState TaskState;
	ETaskStatus TaskStatus;

};
