// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "LXR_Shared.h"
#include "GameFramework/Actor.h" 
#include "Async/AsyncWork.h"


struct FSilhouetteTaskData;

class FProcessSilhouetteTaskAsyncTask : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FProcessSilhouetteTaskAsyncTask>;

public:
	FProcessSilhouetteTaskAsyncTask(FSilhouetteTaskData& InSilhouetteTaskData)
		: Data(&InSilhouetteTaskData)
	{
	}

protected:

	FSilhouetteTaskData* Data;
	
	const UWorld* GetWorld() const;
	TStatId GetStatId() const;

	void DoWork();

};
