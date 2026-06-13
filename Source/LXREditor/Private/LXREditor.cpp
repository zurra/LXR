// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#include "LXREditor.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetTypeActions_LXR.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

#define LOCTEXT_NAMESPACE "FLXREditorModule"

void FLXREditorModule::StartupModule()
{
	// Register
	IAssetTools& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	//AssetCategory
	LXRAssetCategory = AssetToolsModule.RegisterAdvancedAssetCategory(FName(TEXT("LXR")), FText::FromString("LXR"));
	TSharedPtr<FAssetTypeActions_LXR> AssetTypeAction = MakeShareable(new FAssetTypeActions_LXR(LXRAssetCategory));
	AssetToolsModule.RegisterAssetTypeActions(AssetTypeAction.ToSharedRef());


	///source https://www.orfeasel.com/creating-custom-editor-assets/
	StyleSet = MakeShareable(new FSlateStyleSet("LXRStyleSet"));

	FString ContentDir = IPluginManager::Get().FindPlugin("LXR")->GetBaseDir();

	StyleSet->SetContentRoot(ContentDir);
	FSlateImageBrush* ThumbnailBrush = new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("Resources/ClassObjectIcon128"), TEXT(".png")), FVector2D(128.f, 128.f));

	if (ThumbnailBrush)
	{
		StyleSet->Set("ClassThumbnail.LXRMethodObject", ThumbnailBrush);
		FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
	}
}

void FLXREditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FSlateStyleRegistry::UnRegisterSlateStyle(StyleSet->GetStyleSetName());

}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FLXREditorModule, LXREditor)
