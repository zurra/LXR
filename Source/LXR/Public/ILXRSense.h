// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ILXRSense.generated.h"

// This class does not need to be modified.
UINTERFACE()
class ULXRSense : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class LXR_API ILXRSense
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	// Override if you set ULXRLightSenseComponent's SenseTraceTransform to Custom  
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="LXR|Sense")
	void GetLightSenseTraceLocationAndDirection(FVector& Location, FRotator& Rotator);
};
