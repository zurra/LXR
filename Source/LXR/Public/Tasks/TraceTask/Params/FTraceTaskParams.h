// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "LXR_Shared.h"
#include "FTraceTaskParams.generated.h"

USTRUCT(BlueprintType)
struct FTraceTaskParams
{
	GENERATED_BODY()

public:
	virtual ~FTraceTaskParams() = default;
	FTraceTaskParams() = default;

	virtual void SetQuality(ECheckQuality Quality)
	{
	}

protected:
	// Default quality value
	static constexpr ECheckQuality DefaultQuality = ECheckQuality::Medium;
};

