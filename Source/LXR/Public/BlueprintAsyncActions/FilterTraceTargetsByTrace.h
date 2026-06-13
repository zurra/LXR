// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "LXRSubsystem.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "CollisionQueryParams.h"
#include "FilterTraceTargetsByTrace.generated.h"


UCLASS()
class LXR_API UFilterTraceTargetsByTrace : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnTraceTargetOperationDone OnTraceTargetOperationDone;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "LXR|Async|QueryLXR")
	static UFilterTraceTargetsByTrace* FilterTraceTargetsByTrace_Async(const UObject* WorldContextObject, const FVector& FilterTraceStartLocation,
	                                                                   const TSet<FVector>& TraceTargets, TArray<AActor*> ActorsToIgnore,
	                                                                   ECollisionChannel TraceChannel = ECC_Visibility);

	void TaskDone(const TSet<FVector>& Targets);
	// UBlueprintAsyncActionBase interface
	virtual void Activate() override;
	//~UBlueprintAsyncActionBase interface

	virtual UWorld* GetWorld() const override;

private:
	UPROPERTY()
	TObjectPtr<UObject> WorldContextObject;
	UPROPERTY()
	TArray<TObjectPtr<AActor>> ActorsToIgnore;

	FVector StartLocation;
	TSet<FVector> TraceTargets;
	TSet<FVector> FilteredTargets;
	ECollisionChannel TraceChannel;
	FCollisionQueryParams Params;
};
