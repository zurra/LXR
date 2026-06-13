// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CollisionQueryParams.h"
#include "LXRDetectionComponent.h"
#include "LXRSubsystem.h"
#include "LXR_Shared.h"
#include "Tasks/TraceTask/Data/FSenseTaskData.h"
#include "Tasks/TraceTask/Params/FConeTraceTaskParams.h"
#include "LXRLightSenseComponent.generated.h"

class ULXRSourceComponent;

/*Component for detecting light emitted by actors with LXRLightSource component. */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LXR_API ULXRLightSenseComponent : public UActorComponent
{
	GENERATED_BODY()
	friend class ULXRSilhouetteDetectionComponent;

public:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	// Sets default values for this component's properties
	ULXRLightSenseComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void ProcessSenseTask();
	
	/**
	 * @brief Debug options
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Sense|Debug")
	FQueryLXRDebugOptions DebugOptions;

	//Location and Direction to use for the sense cone.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Sense")
	ETraceTransform SenseTraceTransform;

	//Socket to use for Sense trace Location and Direction.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Sense", meta=(HideEditConditionToggle, EditCondition = "SenseTraceTransform == ESenseTraceTransform::Socket"))
	FName SocketName;

	//Method to use for tracing checks.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Sense")
	ETaskProcessType SenseCalculationType = ETaskProcessType::Multithread;

	//Sense check quality to use.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Sense")
	ECheckQuality SenseCheckQuality = ECheckQuality::Medium;

	/**
	 * Parameters to use for the Sense Cone Trace shape.
	 * Can be modified if SenseCheckQuality is set to Custom 
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="LXR|Sense", meta=(HideEditConditionToggle, EditCondition = "SenseCheckQuality == ECheckQuality::Custom"))
	FConeTraceTaskParams SenseTraceTaskParams = FConeTraceTaskParams(SenseCheckQuality);

	//Check rate for Sense
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Sense")
	float CheckRate = 0.025;

	//How many trace targets to iterate per check.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Sense")
	int TraceTargetBatchCount = 30;

	//How many targets needs to pass for light to be sensed.
	UPROPERTY(EditAnywhere, Category="LXR|Sense", meta = (DisplayName="Passed Targets Required", ClampMin = "0", ClampMax = "50", UIMin = "0", UIMax = "50"))
	float TargetsRequired = 5;

	//Trace channel to use for traces.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Sense")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	//Assign the LXRMethodObject to use which overrides the specified method.
	// If any of the Method To Use properties are set to UObject then field LXRMethodObject can be set.
	UPROPERTY(EditAnywhere, Category="LXR|Sense",
		meta=(UseComponentPicker, MetaClass="/Script/LXR.LXRMethodObject", BlueprintBaseOnly, EditCondition =
			"IsEnabledMethodToUse == EMethodToUse::UObject || GetSourceActorStateMethodToUse == EMethodToUse::UObject || GetLightComponentStateMethodToUse == EMethodToUse::UObject || IsLightComponentEnabledMethodToUse == EMethodToUse::UObject || GetMyLightComponentsMethodToUse == EMethodToUse::UObject || IgnoreVisibilityActorsMethodToUse == EMethodToUse::UObject"
		))
	FSoftClassPath LXRMethodObject;

	UPROPERTY(BlueprintReadOnly, Category="LXR|Sense|Passed")
	FLinearColor CombinedLXRColor;
	UPROPERTY(BlueprintReadOnly, Category="LXR|Sense|Passed")
	float CombinedLXRIntensity;

	UPROPERTY(BlueprintReadOnly, Category="LXR|Sense|Passed")
	TArray<FVector> PassedTargets;

	//List of Sensed actors.
	UFUNCTION(BlueprintCallable, Category="LXR|Sense")
	TArray<AActor*> GetSensedActors() const;

	//void AddSensedLightFromThread(const TWeakObjectPtr<AActor>& SensedLight,const TArray<int>& PassedComponents,const TArray<int>& PassedTargets);
	void AddSensedLightFromThread(const FLightSourcePassedData& PassedData);
	//void AddSensedLight(const TWeakObjectPtr<AActor>& SensedLight,const TArray<int>& PassedComponents,const TArray<int>& PassedTargets);
	void AddSensedLight(const FLightSourcePassedData& PassedData);

	void RemoveSensedLightFromThread(const TWeakObjectPtr<ULXRSourceComponent>& SensedLight);
	void RemoveSensedLight(const TWeakObjectPtr<ULXRSourceComponent>& SensedLight);
	void ResetLXRSenseArrays();
	void ProcessGeneratedTargets();
	void GetSenseLXR();


	void SenseTaskFinished();
	// TArray<FVector>& IterateChosenTraceTargets();
	TArray<TWeakObjectPtr<ULXRSourceComponent>> GetOctreeLights();

	// TArray<int> PassedTargetsIndexes;

	// TArray<FVector> AllGeneratedTraceTargets;
	// TArray<FVector> ChosenTraceTargets;
	//Actor, PassedComponent, PassedTargets
	// TMap<TWeakObjectPtr<AActor>, TTuple<TArray<int>,TArray<int>>> SensedLightPassedData;

	TSharedPtr<FSenseTaskData> SenseTaskData;

	void GenerateSenseTargets();

protected:
	void GetSenseLocationAndRotation(FVector& OutLocation, FRotator& OutRotator) const;

private:
	void GetSenseLocationAndRotationForSilhouetteSense(AActor* SilhouetteActor, FVector& OutLocation, FRotator& OutRotator) const;

	void CheckAndRemoveNotValidOrStaleLights();

	// void TraceSphereTargets(const FVector& Location);
	// void TraceConeTargets(const FVector& Location);
	//
	// bool TraceTarget(const FVector& Start, const FVector End) const;

	bool RefreshOctreeLights = true;
	bool ForceGenerateNewTraceTargets = false;
	bool IsBackgroundThreadRunning = false;
	bool GotTargetsFromThread = true;
	bool CheckPassedLightsForRelevancy = false;

	int ConeTargetIterator = INDEX_NONE;
	int SphereTargetIterator = INDEX_NONE;
	int TraceTargetIteratorMax = 100;
	int PassedTargetCounter = 0;

	mutable FRWLock RelevantDataLockObject;

	FCollisionQueryParams Params;
	FBoxCenterAndExtent OctreeBoundsTestObject;

	FLinearColor LastCombinedLXRColor;
	FLinearColor CurrentCombinedLXRColor;

	float LastCombinedLXRIntensity;
	float CurrentCombinedLXRIntensity;

	TArray<TWeakObjectPtr<ULXRSourceComponent>> OctreeLights;
	TArray<FSphere> Spheres;
	TArray<FVector> PotentialTraceTargets;
	TArray<FVector> NewTraceTargets;

	TArray<FLinearColor> CombinedLXRColors;
	TArray<float> CombinedLXRIntensities;

	bool IsLightMemoryEnabled;

	UPROPERTY()
	TObjectPtr<class ULXRMethodObject> MethodObject;
};
