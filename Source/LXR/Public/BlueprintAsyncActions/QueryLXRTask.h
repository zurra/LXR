// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.
#pragma once

#include "LXRSubsystem.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "QueryLXRTask.generated.h"


UCLASS()
class LXR_API UQueryLXRTask : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/**
	UQueryLXRTask return Delegate
	* @return TargetsLXR : LXR Amount of passed targets
	* @return PassedData : Contains passed data. 
	*/
	UPROPERTY(BlueprintAssignable)
	FOnQueryLXRDone OnQueryLXRDone;

	/**
	 * @brief Query Points LXR amount, this is async Async
	 * @param Querier Querier of the query, required, used for world reference.  
	 * @param Points Vector array of locations to query.
	 * @param InLights Relevant lights, can be left empty,
	 * then LXRSubsystem will calculate relevant lights.
	 * @return @type delegate @object OnQueryLXRDone   
	 * @functiontypes latent
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Query Locations LXR Async", BlueprintInternalUseOnly = "true", WorldContext = "Querier"), Category = "LXR|Query")
	static UQueryLXRTask* QueryLocationsLXR_Async(const AActor* Querier, const TArray<FVector>& Points, TArray<ULXRSourceComponent*> InLights);

	void TaskDone(const FTargetLXRData& IlluminatedTargets, const TSet<FLightSourcePassedData>& PassedData);
	// UBlueprintAsyncActionBase interface
	virtual void Activate() override;
	//~UBlueprintAsyncActionBase interface


private:
	UPROPERTY()
	TObjectPtr<UObject> WorldContextObject;
	TArray<FVector> Points;
	UPROPERTY()
	TArray<TObjectPtr<ULXRSourceComponent>> Lights;
	FTargetLXRData IlluminatedTargets;
	TSet<FLightSourcePassedData> PassedData;
};
