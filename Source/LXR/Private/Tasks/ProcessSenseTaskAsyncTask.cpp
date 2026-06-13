// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "Tasks/ProcessSenseTaskAsyncTask.h"
#include "LXRFunctionLibrary.h"
#include "LXRSourceComponent.h"
#include "LXRSubsystem.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponent.h"
#include "Engine/World.h"
#if DEBUG_RENDERING
#include "DrawDebugHelpers.h"
#endif


void FProcessSenseTaskAsyncTask::DoWork()
{
	SCOPE_CYCLE_COUNTER(STAT_LightSenseCheck);

	while (Data->TargetIterator > INDEX_NONE)
	{
		const TArray<FVector> TracePoints = Data->GetNextTraceTargets(Data->SenseComponent->TraceTargetBatchCount);
		const bool IsFromThread = Data->IsBackgroundThread;
		ULXRLightSenseComponent* SenseComponent = Data->SenseComponent;
		Data->PassedDatas.Reset();
		Data->StartState();
		for (const auto& SourceComponent : Data->Lights)
		{
			if (!SourceComponent.IsValid()) continue;

			// FQueryLXRDebugOptions options;
			// options.bDebugVisibility = true;
			// FLightSourcePassedData& PassedDataRef = Data->PassedDatas.FindOrAdd(Light);

			FLightSourcePassedData* PassedDataPtr = Data->PassedDatas.Find(SourceComponent);
			if (PassedDataPtr == nullptr)
			{
				auto ElementID = Data->PassedDatas.Add(SourceComponent);
				PassedDataPtr = &Data->PassedDatas[ElementID];
			}

			Data->RelevancyRequiredAmountToPass = Data->SenseComponent->TargetsRequired < 1 ? TracePoints.Num() * Data->SenseComponent->TargetsRequired : Data->SenseComponent->TargetsRequired;
			Data->RelevantRequiredAmountToPass = Data->RelevancyRequiredAmountToPass;


			ULXRMemoryComponent* MemoryComponent = Data->MemoryComponent;
			const bool IsLightSourceEnabled = SourceComponent->IsEnabled();
			const bool UpdateMemory = Data->LightMemoryEnabled && SourceComponent->bIsMemorizable;
			bool RemoveFromMemory = true;

			if (IsLightSourceEnabled || UpdateMemory)
			{
				if (ULXRFunctionLibrary::CheckIsLightRelevant(Data->SenseComponent->GetOwner(), *Data, *PassedDataPtr, Data->SenseComponent->DebugOptions))
				{
					if (ULXRFunctionLibrary::CheckVisibility(Data->SenseComponent->GetOwner(), *Data, *PassedDataPtr, Data->SenseComponent->DebugOptions))
					{
						if (IsLightSourceEnabled)
							IsFromThread ? SenseComponent->AddSensedLightFromThread(*PassedDataPtr) : SenseComponent->AddSensedLight(*PassedDataPtr);

						if (Data->LightMemoryEnabled)
						{
							IsFromThread ? MemoryComponent->AddOrUpdateLightStateFromThread(SourceComponent) : MemoryComponent->AddOrUpdateLightState(SourceComponent);
						}


#if UE_ENABLE_DEBUG_DRAWING
						// if (SenseComponent->DebugOptions.bDebugLXR_OnlyPassed)
						// {
						// 	TSharedPtr<FSenseTaskData> DebugDataCopy = MakeShared<FSenseTaskData>(*Data);
						//
						// 	ULXRFunctionLibrary::DebugOnGameThread([TaskData = DebugDataCopy , SourceComponent, SourceComponent]
						// 	{
						// 		if (TaskData.IsValid())
						// 		{
						// 			if (TaskData->SensedLightPassedData.Contains(SourceComponent))
						// 			{
						// 				for (const auto& SensedLightPassedData : TaskData->SensedLightPassedData)
						// 				{
						// 					for (auto& Pair : SensedLightPassedData.PassedComponentsAndTargets)
						// 					{
						// 						const int CompIdx = Pair.Key;
						// 						if (!SourceComponent->GetMyLightComponents().IsValidIndex(CompIdx)) continue;
						// 						const ULightComponent* Target = SourceComponent->GetMyLightComponents()[CompIdx];
						// 						FLinearColor LightColor;
						//
						// 						// Target->LightColor.ComputeAndFixedColorAndIntensity
						// 						if (Target->bUseTemperature)
						// 						{
						// 							LightColor = FLinearColor::MakeFromColorTemperature(Target->Temperature);
						// 							LightColor *= Target->GetLightColor();
						// 						}
						// 						else
						// 							LightColor = Target->GetLightColor();
						//
						//
						// 						for (auto& TraceIndex : Pair.Value)
						// 						{
						// 							if (!TaskData->SenseComponent->SenseTaskData->TracePoints.IsValidIndex(TraceIndex))
						// 								continue;
						// 							FVector& ChosenTarget = TaskData->SenseComponent->SenseTaskData->TracePoints[TraceIndex];
						// 							const bool bIsDirectionalLight = Target->IsA(UDirectionalLightComponent::StaticClass());
						// 							FVector EndPos = SourceComponent->GetOwner()->GetActorLocation();
						// 							if (bIsDirectionalLight)
						// 							{
						// 								constexpr double DirectionalLightTraceDistance = 15000;
						// 								const FVector DirectionalForwardInverse = Target->GetForwardVector() * -1;
						// 								EndPos = ChosenTarget + DirectionalForwardInverse.GetSafeNormal() * DirectionalLightTraceDistance;
						// 							}
						//
						//
						// 							DrawDebugPoint(SourceComponent->GetWorld(), EndPos, 8, FColor::Yellow, false, TaskData->SenseComponent->CheckRate);
						// 							DrawDebugDirectionalArrow(SourceComponent->GetWorld(), EndPos, SourceComponent->GetActorLocation(), 500, LightColor.ToFColor(true), false,
						// 							                          TaskData->SenseComponent->CheckRate, 0, 1);
						// 							DrawDebugDirectionalArrow(SourceComponent->GetWorld(), EndPos, SourceComponent->GetActorLocation(), 5, FColor::Green, false,
						// 							                          TaskData->SenseComponent->CheckRate, 0, 0);
						// 						}
						// 					}
						// 				}
						// 			}
						// 		}
						// 	});
						// DrawDebugDirectionalArrow(GetWorld(), GetOwner()->GetActorLocation(), LightSourceComponent->GetOwner()->GetActorLocation(), 15, FColor::White, false, 2.5f, 0, 5);
						// if (Data->SensedLightPassedData.Contains(SourceComponent))
						// {
						// 	for (const auto& SensedLightPassedData : Data->SensedLightPassedData)
						// 	{
						// 		for (auto& Pair : SensedLightPassedData.PassedComponentsAndTargets)
						// 		{
						// 			const int CompIdx = Pair.Key;
						// 			if (!SourceComponent->GetMyLightComponents().IsValidIndex(CompIdx)) continue;
						// 			const ULightComponent* Target = SourceComponent->GetMyLightComponents()[CompIdx];
						// 			FLinearColor LightColor;
						//
						// 			// Target->LightColor.ComputeAndFixedColorAndIntensity
						// 			if (Target->bUseTemperature)
						// 			{
						// 				LightColor = FLinearColor::MakeFromColorTemperature(Target->Temperature);
						// 				LightColor *= Target->GetLightColor();
						// 			}
						// 			else
						// 				LightColor = Target->GetLightColor();
						//
						//
						// 			for (auto& TraceIndex : Pair.Value)
						// 			{
						// 				if (!SenseComponent->SenseTaskData->TracePoints.IsValidIndex(TraceIndex))
						// 					continue;
						// 				FVector& ChosenTarget = SenseComponent->SenseTaskData->TracePoints[TraceIndex];
						//
						// 				DrawDebugDirectionalArrow(GetWorld(), ChosenTarget, SourceComponent->GetActorLocation(), 5, FColor::Green, false, SenseComponent->CheckRate, 0, 0);
						// 				DrawDebugPoint(GetWorld(), ChosenTarget, 8, FColor::Yellow, false, SenseComponent->CheckRate);
						// 				DrawDebugDirectionalArrow(GetWorld(), ChosenTarget, SourceComponent->GetActorLocation(), 500, LightColor.ToFColor(true), false, SenseComponent->CheckRate, 0, 1);
						// 			}
						// 		}
						// 	}
						// }
#endif
					}
				}
				else
				{
					if (IsLightSourceEnabled)
					{
						IsFromThread ? SenseComponent->RemoveSensedLightFromThread(SourceComponent) : SenseComponent->RemoveSensedLight(SourceComponent);
					}
				}
			}
		}
	}
Data->NextState();
}
