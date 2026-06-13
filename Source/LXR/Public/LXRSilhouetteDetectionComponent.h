// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "LXRDetectionComponent.h"
#include "LXRSubsystem.h"
#include "Components/ActorComponent.h"
#include "Templates/Tuple.h"
#include "LXR_Shared.h"
#include "Tasks/TraceTask/Data/FSilhouetteTaskData.h"
#include "Tasks/TraceTask/Params/FSilhouetteTraceTaskParams.h"
#include "LXRSilhouetteDetectionComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LXR_API ULXRSilhouetteDetectionComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class FLSilhouetteTraceTargetAsyncTask;
	friend class ULXRDetectionComponent;

public:
	void SetParamsByQualityLevels();
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	// Sets default values for this component's properties
	ULXRSilhouetteDetectionComponent();

	// Called when the game starts
	virtual void BeginPlay() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//Render debug shapes, also needs to be enabled on Light Source Component.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Silhouette|Debug")
	bool bDrawDebug = false;

	/**
	 * @brief Debug options
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Silhouette|Debug")
	FQueryLXRDebugOptions DebugOptions;

	// //Amount of segments in a capsule.
	// UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Silhouette", meta=(HideEditConditionToggle, ClampMin = "1", ClampMax = "10", UIMin = "1", UIMax = "10"))
	// int SilhouetteEdges = 6;
	//
	// //Distance per trace group in capsule trace.
	// UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Silhouette")
	// int DistancePerSegment = 300;
	//
	// //Capsule sense distance.
	// UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Silhouette")
	// int SilhouetteDistance = 2000;


	//Quality for Relevancy Check
	UPROPERTY(EditAnywhere, Category="LXR|Silhouette")
	ECheckQuality SilhouetteCheckQuality = ECheckQuality::Medium;

	/**
	 * Parameters to use for the Silhouette Cone Trace shape.
	 * Can be modified if SenseCheckQuality is set to Custom 
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="LXR|Silhouette", meta=(HideEditConditionToggle, EditCondition = "SilhouetteCheckQuality == ECheckQuality::Custom"))
	FSilhouetteTraceTaskParams SilhouetteTraceTaskParams = FSilhouetteTraceTaskParams(SilhouetteCheckQuality);

	//How many targets needs to pass for light to be seen by silhouette detection.
	UPROPERTY(EditAnywhere, Category="LXR|Silhouette", meta = (DisplayName="Passed Targets Required", ClampMin = "1", ClampMax = "50", UIMin = "1", UIMax = "50"))
	float TargetsRequired = 0.5f;

	//Method to use for tracing checks.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Silhouette")
	ETaskProcessType SilhouetteCalculationType = ETaskProcessType::Multithread;

	//Bounds size to check for nearby detection components in octree.  
	UPROPERTY(EditAnywhere, Category="LXR|Silhouette",
		meta=(HideEditConditionToggle, EditCondition = "RelevancyCheckType == ERelevancyCheckType::Octree", ClampMin = "1000", ClampMax = "10000", UIMin = "1000", UIMax = "10000"))
	int OctreeCheckBoundsSize = 2000;

	//How many trace targets to iterate per check.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Silhouette")
	int TraceTargetBatchCount = 30;

	//Location and Direction to use for the sense cone.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Silhouette")
	ETraceTransform TraceTransform;

	//Socket for Trace Transform.
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Silhouette", meta=(HideEditConditionToggle, EditCondition = "TraceTransform == ETraceTransform::Socket"))
    FName TraceTransformSocket;


	//Detection cone Angle.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Silhouette")
	float TraceAngle = 15;

	//Detection cone Angle.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Silhouette")
	float DetectionComponentUpdateCheckInterval = 3;
	
	UPROPERTY(BlueprintAssignable)
	FOnSeenBySilhouette OnSilhouetteSpottedActor;

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> NearbyDetectors;

	//Trace channel to use for traces.
	UPROPERTY(EditAnywhere, Category="LXR|Silhouette|")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	//Assign the LXRMethodObject to use which overrides the specified method.
	// If any of the Method To Use properties are set to UObject then field LXRMethodObject can be set.
	UPROPERTY(EditAnywhere, Category="LXR|Silhouette",
		meta=(UseComponentPicker, MetaClass="/Script/LXR.LXRMethodObject", BlueprintBaseOnly, EditCondition =
			"IsEnabledMethodToUse == EMethodToUse::UObject || GetSourceActorStateMethodToUse == EMethodToUse::UObject || GetLightComponentStateMethodToUse == EMethodToUse::UObject || IsLightComponentEnabledMethodToUse == EMethodToUse::UObject || GetMyLightComponentsMethodToUse == EMethodToUse::UObject || IgnoreVisibilityActorsMethodToUse == EMethodToUse::UObject"
		))
	FSoftClassPath LXRMethodObject;


	// void GenerateNextBatchOfChosenTraceTargetsBySilhouetteActor(const AActor* SilhouetteActor);

protected:
	void DoWorkOnSilhouetteTasks();
	
	void GenerateSilhouetteTargetsForTask(FSilhouetteTaskData& TaskData);
	void ProcessSilhouetteTask(FSilhouetteTaskData& Task);
	void AnalyzeSilhouetteTask(FSilhouetteTaskData& Task);
	void SilhouetteTaskFinished(FSilhouetteTaskData& Task);

	
private:
	void GetOwnerStartLocationAndRotation(FVector& OutLocation, FRotator& OutRotator) const;
	void FillNearbyDetectionComponents();
	void GetSilhouetteCylinderStartAndDirection(const AActor& SilhouetteActor, FVector& CylinderLocation, FRotator& CylinderDirection) const;

	// void ResetSilhouetteArrays();

	// void AddDetector(ULXRDetectionComponent* InComponent);
	// void RemoveDetector(ULXRDetectionComponent* InComponent);

	TArray<TWeakObjectPtr<ULXRSourceComponent>>& GetOctreeLights(FVector& OutLocation);

	UPROPERTY()
	TObjectPtr<ULXRSubsystem> LXRSubsystem;

	FCollisionQueryParams GetCollisionQueryParams(const TArray<AActor*>& ActorsToIgnore) const;
	FBoxCenterAndExtent OctreeBoundsTestObject;

	TArray<FVector> AllGeneratedTraceTargets;

	TArray<TWeakObjectPtr<ULXRSourceComponent>> OctreeLights;

	int GetTargetIterator = INDEX_NONE;

	bool RefreshOctreeLights = true;
	bool ForceGenerateNewTraceTargets = false;
	bool IsBackgroundThreadRunning = false;
	bool GotTargetsFromThread = true;
	bool CheckPassedLightsForRelevancy = false;

	TArray<TWeakObjectPtr<AActor>> DetectorsToRemove;
	TMap<TWeakObjectPtr<AActor>, FSilhouetteTaskData> CurrentSilhouetteTasks;


	mutable FRWLock PassedDataLockObject;
	bool LightLock;

	float DetectionComponentFillTimer;
	UPROPERTY()
	TObjectPtr<class ULXRMethodObject> MethodObject;
};
