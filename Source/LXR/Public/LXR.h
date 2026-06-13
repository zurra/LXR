// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Stats/Stats.h"
#include "Stats/Stats2.h"

DECLARE_STATS_GROUP(TEXT("LXR"), STATGROUP_LXR, STATCAT_Advanced); 
DECLARE_LOG_CATEGORY_EXTERN(LogLXR, Log, All);



class FLXRModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
