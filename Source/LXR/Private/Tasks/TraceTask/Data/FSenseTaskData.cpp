// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "Tasks/TraceTask/Data/FSenseTaskData.h"
#include "LXR_Shared.h"


void FSenseTaskData::DecreaseSourcesRefTimer()
{
	for (auto& Pair : SourcesSensedThisTick)
	{
		Pair.Value--;
	}
}

bool FSenseTaskData::IsSourceSensedThisTick(const TWeakObjectPtr<ULXRSourceComponent> SourceComponent)
{
	if (SourcesSensedThisTick.Contains(SourceComponent))
		return SourcesSensedThisTick[SourceComponent] > 0;

	return false;
}

void FSenseTaskData::SetSourceAsSensed(const FLightSourcePassedData* InData)
{
	if (!SourcesSensedThisTick.Contains(InData->SourceComponent))
		SourcesSensedThisTick.Add(InData->SourceComponent);
	SourcesSensedThisTick[InData->SourceComponent] = 5;
}

bool FSenseTaskData::IsValid() const
{
	return SenseComponent != nullptr;
}

bool FSenseTaskData::operator==(const FSenseTaskData& Other) const
{
	return SenseComponent == Other.SenseComponent;
}

bool FSenseTaskData::operator==(const ULXRLightSenseComponent& InSenseComponent) const
{
	return this->SenseComponent == &InSenseComponent;
}
