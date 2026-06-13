// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#include "AssetTypeActions_LXR.h"

#include "BlueprintEditor.h"
#include "LXREditor.h"
#include "LXRMethodObject.h"

FAssetTypeActions_LXR::FAssetTypeActions_LXR(EAssetTypeCategories::Type InAssetCategory): MyAssetCategory(InAssetCategory)
{
}

FText FAssetTypeActions_LXR::GetName() const
{
	return FText::FromString("LXR Method Object");
}

FColor FAssetTypeActions_LXR::GetTypeColor() const
{
	return FColor::Red;
}

UClass* FAssetTypeActions_LXR::GetSupportedClass() const
{
	return ULXRMethodObject::StaticClass();
}

void FAssetTypeActions_LXR::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (UObject* Object : InObjects)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(Object))
		{
				FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
				TSharedRef< IBlueprintEditor > NewKismetEditor = BlueprintEditorModule.CreateBlueprintEditor(Mode, EditWithinLevelEditor, Blueprint, false);
		}
	}
	
	FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);
	// EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
	//
	// for (auto Object : InObjects)
	// {
	// 	auto NewAsset = Cast<ULXRMethodObject>(Object);
	// 	if (NewAsset != nullptr)
	// 	{
	// 		TSharedRef<FLXREditorModule> NewEditor(new FLXREditorModule());
	// 		NewEditor->InitEditor(Mode, EditWithinLevelEditor, NewAsset);
	// 	}
	// }
}

uint32 FAssetTypeActions_LXR::GetCategories()
{
	IAssetTools& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	return AssetToolsModule.RegisterAdvancedAssetCategory(FName(TEXT("LXR")), FText::FromString("LXR"));;
}
