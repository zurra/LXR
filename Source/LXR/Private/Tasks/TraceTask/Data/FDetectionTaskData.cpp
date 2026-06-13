// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.
#pragma once
#include "Tasks/TraceTask/Data/FDetectionTaskData.h"
#include "LXRDetectionComponent.h"

// void FDetectionTaskData::GetNextRelevantCheckLightBatch(TArray<TWeakObjectPtr<ULXRSourceComponent>>& OutLightBatch)
// {
// 	if (!RelevantLights.IsValidIndex(RelevantLightIndex))
// 		RelevantLightIndex = RelevantLights.Num() - 1;
//
// 	if (RelevantLights.Num() == 0)
// 	{
// 		RelevantLightIndex = -1;
// 		return;
// 	}
//
// 	for (; RelevantLightIndex >= 0; --RelevantLightIndex)
// 	{
// 		if (!RelevantLights[RelevantLightIndex].IsValid())
// 		{
// 			RelevantLights.RemoveAtSwap(RelevantLightIndex);
// 			continue;
// 		}
//
// 		if (RelevantLights[RelevantLightIndex]->GetOwner() == DetectionComponent->GetOwner())
// 			continue;
//
// 		OutLightBatch.AddUnique(RelevantLights[RelevantLightIndex]);
// 		if (OutLightBatch.Num() == RelevantLightBatchCount)
// 			break;
// 	}
//
//
// 	if (OutLightBatch.Num() != RelevantLightBatchCount && RelevantLightIndex == -1 && RelevantLights.Num() < RelevantLightBatchCount)
// 	{
// 		for (RelevantLightIndex = RelevantLights.Num() - 1; RelevantLightIndex >= 0; --RelevantLightIndex)
// 		{
// 			if (RelevantLights[RelevantLightIndex].IsValid())
// 			{
// 				OutLightBatch.AddUnique(RelevantLights[RelevantLightIndex]);
// 				RelevantLightIndex--;
// 				if (OutLightBatch.Num() == RelevantLightBatchCount)
// 					break;
// 			}
// 		}
// 	}
// }


void FDetectionTaskData::DecreaseSourcesRefTimer()
{
	for (auto& Pair : SourcesDetectedThisTick)
	{
		Pair.Value--;
	}
}

bool FDetectionTaskData::IsSourceStillRelevant(int EntryIndex)
{
	FLightEntry& Entry = DetectionComponent->Entries[EntryIndex];
	if (!Entry.IsValid()) return false;

	if (Entry.Light->bAlwaysRelevant)
		return true;

	if (SourcesDetectedThisTick.Contains(EntryIndex))
		return SourcesDetectedThisTick[EntryIndex] > 0;

	return false;
}

void FDetectionTaskData::SetSourceAsDetected(int EntryIndex)
{
	if (!SourcesDetectedThisTick.Contains(EntryIndex))
		SourcesDetectedThisTick.Add(EntryIndex);
	SourcesDetectedThisTick[EntryIndex] = 3;

	ULXRSourceComponent& SourceComponent = *DetectionComponent->Entries[EntryIndex].Light;
	if (SourceComponent.bAddDetected && DetectionComponent->bAddToSourceWhenDetected)
	{
		if (!SourceComponent.DetectedActors.Contains(DetectionComponent->GetOwner()))
		{
			SourceComponent.DetectedActors.Add(DetectionComponent->GetOwner());
		}
	}
}

bool FDetectionTaskData::operator==(const FDetectionTaskData& Other) const
{
	return DetectionComponent == Other.DetectionComponent;
}

bool FDetectionTaskData::operator==(const ULXRDetectionComponent& InDetectionComponent) const
{
	return this->DetectionComponent == &InDetectionComponent;
}


bool FDetectionTaskData::IsValid() const
{
	return DetectionComponent != nullptr;
}
//
// void FDetectionTaskData::CheckAndRemoveNonRelevantLights()
// {
// 	if (TaskLock) return;
// 	for (int i = 0; i < RelevantLightsToRemove().Num(); ++i)
// 	{
// 		RemoveRelevantLight(RelevantLightsToRemove()[i]);
// 	}
// 	RelevantLightsToRemove().Reset();
// }

// void FDetectionTaskData::CheckAndAddNewRelevantLights()
// {
// 	if (TaskLock) return;
// 	for (int i = 0; i < RelevantLightsToAdd().Num(); ++i)
// 	{
// 		AddRelevantLight(RelevantLightsToAdd()[i]);
// 	}
// 	RelevantLightsToAdd().Reset();
// }

// Not Used, check RemoveRelevantLight
void FDetectionTaskData::RemovePassedLight(FLightSourcePassedData& PassedData)
{
	if (TaskLock) return;

	TaskLock = true;
	const FLightSourcePassedData* ElementToRemove = nullptr;
	for (FLightSourcePassedData& Element : PassedDatas)
	{
		if (Element.SourceComponent == PassedData.SourceComponent)
		{
			ElementToRemove = &Element;
			break;
		}
	}

	if (ElementToRemove != nullptr)
	{
		if (ElementToRemove->SourceComponent.IsValid())
		{
			if (ElementToRemove->SourceComponent->bAddDetected && DetectionComponent->bAddToSourceWhenDetected)
				ElementToRemove->SourceComponent->DetectedActors.Remove(DetectionComponent->GetOwner());
		}
		PassedDatas.Remove(*ElementToRemove);
	}


	TaskLock = false;
}

void FDetectionTaskData::AddPassedLight(FLightSourcePassedData& PassedData)
{
	if (TaskLock) return;
	FLightSourcePassedData* Data = nullptr;
	for (auto& Element : PassedDatas)
	{
		if (Element.SourceComponent == PassedData.SourceComponent)
		{
			Data = &Element;
			break;
		}
	}

	if (Data == nullptr)
	{
		auto ElementID = PassedDatas.Add(PassedData);
		Data = &PassedDatas[ElementID];
	}

	int EntryIndex = DetectionComponent->
	LightToIndex[Data->SourceComponent.Get()];
	SetSourceAsDetected(EntryIndex);
	Data->PassedComponentsAndTargets = PassedData.PassedComponentsAndTargets;
}

// void FDetectionTaskData::AddRelevantLight(int EntryIndex)
// {
// 	if (TaskLock) return;
// 	
// 	int Index = RelevantLights().Add(EntryIndex);
// 	FLightEntry& Entry = DetectionComponent->Entries[EntryIndex];
// 	Entry.BucketPos = Index;
// 	Entry.Bucket = ELightBucket::Relevant;
// }

// void FDetectionTaskData::RemoveRelevantLight(int EntryIndex)
// {
// 	// if (TaskLock) return;
//
// 	FLightEntry& Entry = DetectionComponent.Get()->Entries[EntryIndex];
// 	const TWeakObjectPtr<ULXRSourceComponent> SourceComponent = Entry.Light;
//
// 	Entry.Flags |= ELightFlags::PendingMove;
// 	Entry.PendingBucket = ELightBucket::Irrelevant;
// 	DetectionComponent->Pending.Add(EntryIndex);
//
// 	if (SourceComponent->bAddDetected && DetectionComponent->bAddToSourceWhenDetected)
// 	{
// 		if (SourceComponent->DetectedActors.Contains(DetectionComponent->GetOwner()))
// 		{
// 			SourceComponent->DetectedActors.RemoveSwap(DetectionComponent->GetOwner());
// 		}
// 	}
// }
