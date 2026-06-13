// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "LXRSubsystem.h"
#include "LXR_Shared.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvQueryTest_LXR.generated.h"

class AActor;
struct FCollisionQueryParams;

UCLASS(MinimalAPI)
class UEnvQueryTest_LXR : public UEnvQueryTest
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category=LXR)
	FQueryLXRDebugOptions DebugOptions;

	UPROPERTY(EditDefaultsOnly, Category=LXR)
	FLinearColor ColorToTest = FColor::White;

	UPROPERTY(EditDefaultsOnly, Category=LXR, AdvancedDisplay)
	FAIDataProviderFloatValue LXRMultiplier;
	
	UEnvQueryTest_LXR(const FObjectInitializer& OI);
	UPROPERTY(EditDefaultsOnly, Category=Trace)
	TSubclassOf<UEnvQueryContext> Context;

	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;

	virtual void PostLoad() override;

protected:
	FText LXRText;
	FTargetLXRData TargetLXR;
};
