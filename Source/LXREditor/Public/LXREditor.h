// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "AssetTypeCategories.h"

class FSlateStyleSet;

class FLXREditorModule : public IModuleInterface
{
public:
	TSharedPtr<FSlateStyleSet> StyleSet;

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
private:
	 EAssetTypeCategories::Type LXRAssetCategory;
};
