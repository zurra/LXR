// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#include "LXR_Shared.h"

#include "LXRDetectionComponent.h"
#include "LXRFunctionLibrary.h"


FLinearColor FTargetLXRData::GetAverageColor() const
{
	return ULXRFunctionLibrary::GetLinearColorArrayAverage(GetAllTargetColorsAsArray());
}

float FTargetLXRData::GetWeightedAverageLuxFromAllTargets(float MinLuxThreshold, float WeightToUseAboveMinLuxThreshold) const
{
	TArray<float> Values = GetAllTargetsAverageLuxValues();
	float Sum = 0.0f;
	float MaxLux = 0.0f;
	int Count = TraceTargets.Num();
	for (float Lux : Values)
	{
		if (Lux > MinLuxThreshold)
		{
			Sum += Lux;
			MaxLux = FMath::Max(MaxLux, Lux);
		}
	}
	if (Count > 0)
	{
		float Average = Sum / Count;
		return (WeightToUseAboveMinLuxThreshold * MaxLux + 0.3f * Average);
	}
	return 0.0f;
}



float FTargetLXRData::GetAverageLuxFromAllTargets() const
{
	TArray<float> Values = GetAllTargetsAverageLuxValues();
	float Sum = 0.0f;
	int Count = TraceTargets.Num();
	for (float Lux : Values)
	{
		Sum += Lux;
	}
	if (Count > 0)
	{
		return Sum / Count;
	}
	return 0;
}

float FTargetLXRData::GetAverageLuxFromPassedTargets() const
{
	TArray<float> Values = GetAllTargetsAverageLuxValues();
	float Sum = 0.0f;
	int Count = TargetsLXR.Num();
	for (float Lux : Values)
	{
		Sum += Lux;
	}
	if (Count > 0)
	{
		return Sum / Count;
	}
	return 0;
}

float FTargetLXRData::GetWeightedAverageLuxFromPassedTargets(float MinLuxThreshold, float WeightToUseAboveMinLuxThreshold) const
{
	TArray<float> Values = GetAllTargetsAverageLuxValues();
	float Sum = 0.0f;
	float MaxLux = 0.0f;
	int Count = TargetsLXR.Num();
	for (float Lux : Values)
	{
		if (Lux > MinLuxThreshold)
		{
			Sum += Lux;
		}
	}
	if (Count > 0)
	{
		float Average = Sum / Count;
		return (WeightToUseAboveMinLuxThreshold * MaxLux + 0.3f * Average);
	}
	return 0.0f;
}

float FTargetLXRData::GetMaxLux() const
{
	TArray<float> Values = GetAllTargetsLuxValues();
	float MaxLux = 0.0f;
	for (float Lux : Values)
	{
		MaxLux = FMath::Max(MaxLux, Lux);
	}
	return MaxLux;
}


float FTargetLXRData::GetSocketWeightedLux(ULXRDetectionComponent& DetectionComponent)
{
	float WeightedLuxSum = 0.0f;
	float TotalWeight = 0.0f;
	int32 SocketsLit = 0;

	for (auto& Pair : TargetsLXR)
	{
		FName Socket = DetectionComponent.GetSocketNameByTraceTargetIndex(Pair.Key);
		float SocketWeight = DetectionComponent.GetSocketWeight(Socket);
		TotalWeight += SocketWeight;
		float Lux = Pair.Value.Intensity;
		if (Lux > DetectionComponent.MinLuxThreshold)
		{
			WeightedLuxSum += Lux * SocketWeight;
			++SocketsLit;
		}
	}

	const int32 TotalSockets = TraceTargets.Num();
	const float CoveragePercent = (TotalSockets > 0) ? (float(SocketsLit) / float(TotalSockets)) * 100.0f : 0.0f;
	float FinalLux = (TotalWeight > 0.0f) ? (WeightedLuxSum / TotalWeight) : 0.0f;

	if (CoveragePercent < DetectionComponent.MinimumCoveragePercent)
	{
		FinalLux *= DetectionComponent.LowCoveragePenaltyMultiplier;
	}

	return FinalLux;
}
