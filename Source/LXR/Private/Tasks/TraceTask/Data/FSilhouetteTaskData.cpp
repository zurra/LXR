// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "Tasks/TraceTask/Data/FSilhouetteTaskData.h"

bool FSilhouetteTaskData::operator==(const FSilhouetteTaskData& Other) const
{
	return DetectionComponent == Other.DetectionComponent;
}

bool FSilhouetteTaskData::operator==(const ULXRDetectionComponent& InDetectionComponent) const
{
	return this->DetectionComponent == &InDetectionComponent;
}
