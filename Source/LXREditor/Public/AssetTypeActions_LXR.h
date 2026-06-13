// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"


class LXREDITOR_API FAssetTypeActions_LXR : public FAssetTypeActions_Base
{
public:
	FAssetTypeActions_LXR(EAssetTypeCategories::Type InAssetCategory);

	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;

	virtual uint32 GetCategories() override;

private:
	EAssetTypeCategories::Type MyAssetCategory;
};
