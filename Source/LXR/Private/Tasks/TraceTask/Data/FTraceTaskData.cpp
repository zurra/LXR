// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "Tasks/TraceTask/Data/FTraceTaskData.h"

FTraceTaskData::FTraceTaskData(const TArray<FVector>& InTracePoints): TargetIterator(0), bValidTick(false), IsBackgroundThread(false)
{
	Reset(InTracePoints);
}

void FTraceTaskData::Reset(const TArray<FVector>& InTracePoints)
{
	SetNewTracePointsAndResetIterator(InTracePoints);
	ChosenTracePoints = TracePoints;
}

ETaskState FTraceTaskData::GetTaskState() const
{
	return TaskState;
}

ETaskStatus FTraceTaskData::GetTaskStatus() const
{
	return TaskStatus;
}

void FTraceTaskData::NextState()
{
	const uint8 Count = static_cast<uint8>(ETaskState::Max);
	const uint8 Next = (static_cast<uint8>(TaskState) + 1) % Count;
	TaskState = static_cast<ETaskState>(Next);
	TaskStatus = ETaskStatus::Waiting;
}

void FTraceTaskData::SetState(ETaskState NewState)
{
	TaskState = NewState;
	TaskStatus = ETaskStatus::Waiting;
}

void FTraceTaskData::StartState()
{
	TaskStatus = ETaskStatus::Started;
}

void FTraceTaskData::FinishState()
{
	TaskStatus = ETaskStatus::Finished;
}

void FTraceTaskData::SetNewTracePointsAndResetIterator(const TArray<FVector>& InTracePoints)
{
	TracePoints = InTracePoints;
	TargetIterator = TracePoints.Num() - 1;
}

void FTraceTaskData::SetLights(const TArray<TWeakObjectPtr<ULXRSourceComponent>>& InLights)
{
	Lights = InLights;
}

TArray<FVector>& FTraceTaskData::GetNextTraceTargets(const int& TargetBatchCount, bool All)
{
	if (All)
	{
		TargetIterator = INDEX_NONE;
		ChosenTracePoints = TracePoints;
	}
	else
	{
		if (TracePoints.Num() == 0) return ChosenTracePoints;
		ChosenTracePoints.Reset();
		const int Max = FMath::Max(TargetIterator - TargetBatchCount, 0);
		for (; TargetIterator >= Max; --TargetIterator)
		{
			if (TargetIterator == -1)
				break;

			ChosenTracePoints.Add(TracePoints[TargetIterator]);
		}
	}

	return ChosenTracePoints;
}
