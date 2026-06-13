// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "LXRMethodObject.h"
#include "Factories/BlueprintFactory.h"
#include "Factories/Factory.h"
#include "LXRMethodObjectFactory.generated.h"

/**
 * 
 */
UCLASS()
class LXREDITOR_API ULXRMethodObjectFactory : public UBlueprintFactory
{
	GENERATED_BODY()

	ULXRMethodObjectFactory();

	// virtual uint32 GetMenuCategories() const override;
public:
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
private:
	/** Class of Blueprint RetroDebugWindowWidget*/
	TSubclassOf<ULXRMethodObject> MethodObjectBPClass;
};
