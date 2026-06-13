// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#include "LXRMethodObjectFactory.h"
#include "IAssetTools.h"
#include "KismetCompilerModule.h"
#include "LXRMethodObject.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/ConstructorHelpers.h"


ULXRMethodObjectFactory::ULXRMethodObjectFactory(): Super()
{
	bCreateNew = true;
	bEditAfterNew = true;

	SupportedClass = ULXRMethodObject::StaticClass();

	// FString MethodObjectPath = IPluginManager::Get().FindPlugin("LXR")->GetContentDir()+"/LXR/Classes/U_LXR_MethodObject_Base";


	static ConstructorHelpers::FClassFinder<ULXRMethodObject> LxrMethodObjectClassFinder(TEXT("/LXR/Classes/U_LXR_MethodObject_Base"));
	// static ConstructorHelpers::FClassFinder<ULXRMethodObject> LxrMethodObjectClassFinder(TEXT("/LXR/Classes/U_LXR_MethodObject_Base"));
	if (LxrMethodObjectClassFinder.Succeeded())
	{
		UE_LOG(LogTemp, Log, TEXT("Found  %s"), *LxrMethodObjectClassFinder.Class->GetName());
		MethodObjectBPClass = LxrMethodObjectClassFinder.Class;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("LxrMethodObjectClass not found... LXR Method objects have to be added by making a new BP class."));
	}
	ParentClass = MethodObjectBPClass;
}

bool ULXRMethodObjectFactory::ConfigureProperties()
{
	return true;
}

// uint32 ULXRMethodObjectFactory::GetMenuCategories() const
// {
// 	// return EAssetTypeCategories::Blueprint;
// 	IAssetTools& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
// 	return AssetToolsModule.RegisterAdvancedAssetCategory(FName(TEXT("LXR2")), FText::FromString("LXR2"));;
// }


UObject* ULXRMethodObjectFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	UClass* BlueprintClass = nullptr;
	UClass* BlueprintGeneratedClass = nullptr;

	IKismetCompilerInterface& KismetCompilerModule = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>("KismetCompiler");
	KismetCompilerModule.GetBlueprintTypesForClass(ParentClass, BlueprintClass, BlueprintGeneratedClass);

	return FKismetEditorUtilities::CreateBlueprint(ParentClass, InParent, InName, BPTYPE_Normal, BlueprintClass, BlueprintGeneratedClass, NAME_None);
}
