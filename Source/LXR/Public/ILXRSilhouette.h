// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ILXRSilhouette.generated.h"

// This class does not need to be modified.
UINTERFACE()
class ULXRSilhouette : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class LXR_API ILXRSilhouette
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	// Override if you set ULXRLightSilhouetteComponent's SilhouetteTraceTransform to Custom  
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="LXR|Silhouette")
	void GetSilhouetteTraceLocationAndDirection(FVector& Location, FRotator& Rotator);
};
