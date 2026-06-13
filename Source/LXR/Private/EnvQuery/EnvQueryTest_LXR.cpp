// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#include "EnvQuery/EnvQueryTest_LXR.h"
#include "DrawDebugHelpers.h"
#include "LXRFunctionLibrary.h"
#include "LXRSourceComponent.h"
#include "LXRSubsystem.h"
#include "LXR_Shared.h"
#include "Engine/World.h"
#include "UObject/UObjectIterator.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "Tasks/TraceTask/Data/FTraceTaskData.h"


UEnvQueryTest_LXR::UEnvQueryTest_LXR(const FObjectInitializer& OI)
{
	Context = UEnvQueryContext_Querier::StaticClass();
	Cost = EEnvTestCost::Medium;
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	LXRMultiplier.DefaultValue = 1;
}

void UEnvQueryTest_LXR::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* DataOwner = QueryInstance.Owner.Get();
	BoolValue.BindData(DataOwner, QueryInstance.QueryID);

	FloatValueMin.BindData(DataOwner, QueryInstance.QueryID);
	float MinThresholdValue = FloatValueMin.GetValue();

	FloatValueMax.BindData(DataOwner, QueryInstance.QueryID);
	float MaxThresholdValue = FloatValueMax.GetValue();

	LXRMultiplier.BindData(DataOwner,QueryInstance.QueryID);
	float LXRMultiplierValue = LXRMultiplier.GetValue();

	TArray<FVector> ContextLocations;
	if (!QueryInstance.PrepareContext(Context, ContextLocations))
	{
		return;
	}

	AActor* OwnerActor = nullptr;
	TArray<AActor*> OwnerActors;
	if (QueryInstance.PrepareContext(Context, OwnerActors))
	{
		OwnerActor = OwnerActors[0];
	}

	// ULXRDetectionComponent* DetectionComponent = OwnerActor->FindComponentByClass<ULXRDetectionComponent>();
	// if (DetectionComponent->IsValidLowLevel())
	// {
	TArray<TWeakObjectPtr<ULXRSourceComponent>> LightBatch;
	bool bSoloFound = false;

	if (ULXRSubsystem* LXRSubsystem = OwnerActor->GetWorld()->GetSubsystem<ULXRSubsystem>())
	{
		bSoloFound = LXRSubsystem->bSoloFound;
		TArray<TWeakObjectPtr<ULXRSourceComponent>> AllLights = LXRSubsystem->GetAllLights();
		if (AllLights.Num() > 0)
		{
			LightBatch = AllLights;
		}
	}

	if (LightBatch.Num() == 0)
	{
		for (TObjectIterator<ULXRSourceComponent> Itr; Itr; ++Itr)
		{
			if (Itr->bSolo)
				bSoloFound = true;
			LightBatch.Add(*Itr);
		}
	}


	TArray<FVector> ItemLocations;
	QueryInstance.GetAllAsLocations(ItemLocations);

	//Actor, PassedComponent, PassedTargets
	TSet<FLightSourcePassedData> PassedData;

	for (TWeakObjectPtr<ULXRSourceComponent>& SourceComponent : LightBatch)
	{
		if (!SourceComponent.IsValid()) continue;
		if (!SourceComponent->IsEnabled()) continue;

		if (bSoloFound)
		{
			if (!SourceComponent->bSolo) continue;
		}
		FLightSourcePassedData NewData = FLightSourcePassedData(SourceComponent);
		FTraceTaskData TraceTaskData = FTraceTaskData(ItemLocations);
		TraceTaskData.TraceType = EDetectionType::Environment;

		if (ULXRFunctionLibrary::CheckIsLightRelevant(GetWorld(), TraceTaskData, NewData, DebugOptions))
		{
			for (auto& Pair : NewData.PassedComponentsAndTargets)
			{
				const FVector TraceTarget = ItemLocations[Pair.Key];
				if (DebugOptions.bDebugRelevancy)
				{
					DrawDebugPoint(GetWorld(), TraceTarget, 5, FColor::Yellow, false, 0.5f, 0);
				}
			}

			if (ULXRFunctionLibrary::CheckVisibility(GetWorld(), TraceTaskData, NewData, DebugOptions))
			{
				PassedData.Add(NewData);
			}
		}
	}
	// TMap<int, TTuple<float, FLinearColor>> IlluminatedTargets;

	FTargetLXRData& MutableTargetLXR = const_cast<UEnvQueryTest_LXR*>(this)->TargetLXR;
	if (PassedData.Num() > 0)
	{
		ULXRFunctionLibrary::GetLux(GetWorld(), ItemLocations, PassedData, MutableTargetLXR, DebugOptions, LXRMultiplierValue);
	}

	
	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector ItemLocation = GetItemLocation(QueryInstance, It.GetIndex());

		// for (int32 ContextIndex = 0; ContextIndex < ContextLocations.Num(); ContextIndex++)
		// {
		// It.SetScore(TestPurpose, FilterType, 1, MinThresholdValue, MaxThresholdValue);

		int IndexOfTarget = ItemLocations.Find(ItemLocation);
		if (TargetLXR.TargetsLXR.Contains(IndexOfTarget))
		{
			float LXRIntensity = TargetLXR.TargetsLXR[IndexOfTarget].Intensity;
			if (ColorToTest.Equals(FLinearColor::White))
			{
				It.SetScore(TestPurpose, FilterType, FMath::Min(LXRIntensity,1), MinThresholdValue, MaxThresholdValue);
				continue;
			}
			else
			{
				FLinearColor LXRColor = TargetLXR.TargetsLXR[IndexOfTarget].Color;
				if (ULXRFunctionLibrary::ColorApproximatelyEqualColor(ColorToTest, LXRColor))
				{
					It.SetScore(TestPurpose, FilterType, LXRIntensity, MinThresholdValue, MaxThresholdValue);
					continue;
				}
			}
		}

		// It.SetScore(TestPurpose, FilterType, 0, MinThresholdValue, MaxThresholdValue);
	}

	// for (int i = ItemLocations.Num() - 1; i >= 0; --i)
	// {
	// 	bool Relevant = DetectionComponent->CheckIsLightRelevant(*Light,PassedComponents,PassedTargets,EDetectionType::Location);
	// 	if(!Relevant)
	// 	{
	// 		
	// 	}
	// }

	// for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	// {
	// 	const FVector ItemLocation = GetItemLocation(QueryInstance, It.GetIndex());
	//
	// 	for (int32 ContextIndex = 0; ContextIndex < ContextLocations.Num(); ContextIndex++)
	// 	{
	// 		DetectionComponent.
	// 	}
	// }
}

// }


FText UEnvQueryTest_LXR::GetDescriptionTitle() const
{
	return FText::FromString("LXR:  on ITEM");
}

FText UEnvQueryTest_LXR::GetDescriptionDetails() const
{
	return FText::FromString("LXR:  on ITEM DETAILS");
}

void UEnvQueryTest_LXR::PostLoad()
{
	Super::PostLoad();
}
