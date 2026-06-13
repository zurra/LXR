// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CollisionQueryParams.h"
#include "LXRMemoryComponent.h"
#include "LXRSourceComponent.h"
#include "Math/GenericOctree.h"
#include "Blueprint/UserWidget.h"
#include "Templates/Tuple.h"
#include "LXR_Shared.h"
#include "LXRSubsystem.h"
#include "Tasks/TraceTask/Data/FDetectionTaskData.h"
#include "Tasks/TraceTask/Data/FTraceTaskData.h"
#include "LXRDetectionComponent.generated.h"

enum class ELightBucket : uint8
{
	SmartNear = 0,
	SmartMid = 1,
	SmartFar = 2,
	Octree = 3,
	Relevant = 4,
	Irrelevant = 5,
	All = 6,
	None = 7
};

enum class ELightBucketDetectionTask : uint8
{
	RelevantLights = 0,
	RelevantLightsToRemove = 1,
	RelevantLightsToAdd = 2,
};


enum class ELightFlags : uint8
{
	None = 0,
	PendingAdd = 1 << 0,
	PendingMove = 1 << 1,
	PendingRemove = 1 << 2
};

ENUM_CLASS_FLAGS(ELightFlags)

struct FLightEntry
{
	FLightEntry(): Bucket(), PendingBucket(), Flags(), BucketPos(0)
	{
		bAlive = false;
	}
	
	FLightEntry(TWeakObjectPtr<ULXRSourceComponent> InLight): Bucket(ELightBucket::None), Flags(), BucketPos(0)
	{
		Light = InLight;
		bAlive = true;
	}


	TWeakObjectPtr<ULXRSourceComponent> Light;

	ELightBucket Bucket;
	ELightBucket PendingBucket;
	ELightFlags Flags;
	int32 BucketPos;
	bool bAlive = false;

	bool operator==(const FLightEntry& Other) const
	{
		return Light == Other.Light;
	}

	bool operator==(ULXRSourceComponent& OtherSourceComponent) const
	{
		return Light.Get() == &OtherSourceComponent;
	}

	bool IsValid()
	{
		return Light.IsValid();
	}
	
};

UENUM()
enum class ERelevancyCheckType
{
	//Use property RelevancyCheckRate as check rate.
	Fixed UMETA(DisplayName = "Fixed"),

	// Calculate rate from distance to light.
	// Lights will be categorized in three different category:
	// Near: Distance to light is less than RelevancySmartDistanceMin, light will be checked 4 times in second.
	// Mid: Distance to light is between RelevancySmartDistanceMin and RelevancySmartDistanceMax, light will be checked twice in second.
	// Far: Distance to light is more than RelevancySmartDistanceMax, light will be checked every second.
	Smart UMETA(DisplayName = "Smart"),

	// Use octree to iterate nearby lights for relevancy defined by RelevancyCheckRate property.
	Octree UMETA(DisplayName = "Octree")
};

UENUM()
enum class ELuxReturnType
{
	// Returns the average of targets, not just passed.
	AverageByTargets UMETA(DisplayName = "Average By All Targets"),

	// Returns the average of passed targets, this is usually the best option to keep.
	AverageByPassedTargets UMETA(DisplayName = "Average By Passed Targets"),

	// Returns the MAX value of all targets;	
	MaxOfPassedTargets UMETA(DisplayName = "Max of Passed Targets"),

	// Returns the Socket weighted importance;	
	WeightedSocketImportance UMETA(DisplayName = "Weighted Socket Importance")
};


/*Component for detecting light emitted by actors with LXRLightSource component. */
UCLASS(Blueprintable, ClassGroup=(LXR), meta=(BlueprintSpawnableComponent))
class LXR_API ULXRDetectionComponent : public UActorComponent
{
	GENERATED_BODY()
	friend class ULXRLightSenseComponent;
	friend class ULXRSilhouetteDetectionComponent;
	friend class UEnvQueryTest_LXR;
	friend class FProcessDetectionTaskAsyncTask;
	friend class ULXRDebugSubsystem;
	friend struct FTargetLXRData;
	friend struct FDetectionTaskData;

public:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// Sets default values for this component's properties
	ULXRDetectionComponent();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	// Use Server Authority for Detection
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Detection|Networking")
	bool bUserServerAuthority = false;

	// Let Client also calculate detection
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Detection|Networking", meta=(HideEditConditionToggle, EditCondition = "bUserServerAuthority == true"))
	bool bUseClientSideCalculation = false;

	// Intensity change tolerance to send update to client
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Detection|Networking", meta=(HideEditConditionToggle, EditCondition = "bUserServerAuthority == true"))
	float IntensityUpdateTolerance = 0.1f;

	// Color change tolerance to send update to client
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Detection|Networking", meta=(HideEditConditionToggle, EditCondition = "bUserServerAuthority == true"))
	float ColorUpdateTolerance = 0.1f;

	// Debug Options to use
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Detection|Debug")
	FQueryLXRDebugOptions DebugOptions;

	//Debug Render Octree lights 
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Detection|Debug")
	bool bDebugOctreeLights = false;

	//Draw debug sphere for each VectorArray position.
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Debug")
	bool bDebugVectorArray = false;

	//Which Skeletal Mesh Component use if many are present.
	UPROPERTY(EditAnywhere, Category="LXR|Detection", meta=(UseComponentPicker, AllowedClasses="/Script/Engine.SkeletalMeshComponent"))
	FComponentReference SkeletalMeshComponent;

	//Use preset sockets or custom one..
	UPROPERTY(EditAnywhere, Category="LXR|Detection")
	ESocketPreset SocketPreset = ESocketPreset::UE5_Mannequin;

	//Bones or sockets to use as detection targets 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Detection", meta=(HideEditConditionToggle, EditCondition = "SocketPreset == ESocketPreset::Custom"))
	TMap<FName, float> TargetSockets;

	UPROPERTY(BlueprintReadOnly, Category="LXR|Detection")
	TArray<FName> CachedSockets;

	//Detection targets in local space to use for VectorArray type.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LXR|Detection")
	TArray<FVector> TargetVectors;

	//This actor can be added to source component DetectedActors list.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Detection")
	bool bAddToSourceWhenDetected = false;

	//Quality for Relevancy Check
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Relevancy")
	ECheckQuality RelevancyCheckQuality = ECheckQuality::Medium;

	//Target type to use for relevancy detection.
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Relevancy")
	ETraceTarget RelevancyTargetType = ETraceTarget::ActorLocation;

	//Method to use for checking if light is relevant
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Relevancy")
	ERelevancyCheckType RelevancyCheckType = ERelevancyCheckType::Smart;

	//Min distance after which check rate will be reduced.  
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Relevancy|Quality",
		meta=(EditCondition = "RelevancyCheckType == ERelevancyCheckType::Smart && RelevancyCheckQuality == ECheckQuality::Custom"))
	float RelevancySmartDistanceMin = 1000;

	//Max distance after which check rate will be greatly reduced.
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Relevancy|Quality",
		meta=(EditCondition = "RelevancyCheckType == ERelevancyCheckType::Smart && RelevancyCheckQuality == ECheckQuality::Custom"))
	float RelevancySmartDistanceMax = 2500;

	//Max distance to check if Directional Light sees owner.
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Directional Light")
	float DirectionalLightTraceDistance = 10000;

	UPROPERTY(EditAnywhere, Category="LXR|Detection|Smoothing")
	float LuxValueSmoothSpeed = 50.f;

	//Determines how Lux capture data is summarized.
	//Choose between averaging all sockets, only lit sockets, taking the brightest value, or using socket-specific importance weighting.
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Lux")
	ELuxReturnType LuxReturnType = ELuxReturnType::MaxOfPassedTargets;

	UPROPERTY(EditAnywhere, Category="LXR|Detection|Lux")
	bool GetWeightedAverage = false;

	UPROPERTY(EditAnywhere, Category="LXR|Detection|Lux", meta=(EditCondition = "GetWeightedAverage == true"))
	bool WeightToUseAboveMinLuxThreshold = 1.0f;

	// Lux amount target must have to be considered as valid for Lux calculation
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Lux",
		meta=(EditCondition = "LuxReturnType == ELuxReturnType::AverageByPassedTargets || LuxReturnType == ELuxReturnType::AverageByTargets && GetWeightedAverage == true"))
	float WeightedMinLuxThreshold = 1.0f;

	// Lux amount target must have to be considered as valid for Lux calculation
	// Used for Weighted Socket 
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Lux", meta=(EditCondition = "LuxReturnType == ELuxReturnType::WeightedSocketImportance"))
	float MinLuxThreshold = 1.0f;

	// % of lit sockets required
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Lux", meta=(EditCondition = "LuxReturnType == ELuxReturnType::WeightedSocketImportance"))
	float MinimumCoveragePercent = 30.0f;

	// Multiplier if low coverage
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Lux", meta=(EditCondition = "LuxReturnType == ELuxReturnType::WeightedSocketImportance"))
	float LowCoveragePenaltyMultiplier = 0.5f;

	//Divider for Smart check rate.
	// Near check rate is 5/RelevancySmartCheckRateDivider in second
	// Mid: light will be 2/RelevancySmartCheckRateDivider twice in second.
	// Far: light will be 1/RelevancySmartCheckRateDivider every second.
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Relevancy|Quality",
		meta=(HideEditConditionToggle, EditCondition = "RelevancyCheckType == ERelevancyCheckType::Smart && RelevancyCheckQuality == ECheckQuality::Custom", ClampMin = "1",
			ClampMax = "10", UIMin = "1", UIMax = "10"))
	float RelevancySmartCheckRateDivider = 1.f;

	//Rate for checking for new relevant lights.  
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Relevancy|Quality",
		meta=(HideEditConditionToggle, EditCondition = "RelevancyCheckType != ERelevancyCheckType::Smart && RelevancyCheckQuality == ECheckQuality::Custom"))
	float RelevancyCheckRate = 0.05f;

	//Bounds size to check for nearby lights.  
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Relevancy|Quality",
		meta=(HideEditConditionToggle, EditCondition = "RelevancyCheckType == ERelevancyCheckType::Octree && RelevancyCheckQuality == ECheckQuality::Custom", ClampMin = "1000",
			ClampMax = "10000", UIMin = "1000", UIMax = "10000"))
	int RelevancyOctreeCheckBoundsSize = 2000;

	//How many lights we check for relevancy per check.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Detection|Relevancy")
	int RelevancyLightBatchCount = 100;

	// //Use location difference for relevancy checks.
	// UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Detection|Relevancy", meta=(HideEditConditionToggle, EditCondition = "RelevancyCheckType != ERelevancyCheckType::Octree"))
	// bool bUseLocationChange = false;
	//
	// //Difference from last relevancy check.  
	// UPROPERTY(EditAnywhere, Category="LXR|Detection|Relevancy", meta=(HideEditConditionToggle, ClampMin = "100", ClampMax = "1000", UIMin = "100", UIMax = "1000", EditCondition = "bUseLocationChange == true"))
	// int RelevancyLocationThreshold = 200;

	//Percentage of how many targets needs to pass relevancy check for light to be relevant for visibility. 
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Relevancy",
		meta = (DisplayName="Passed Targets Required", ClampMin = "0.1", ClampMax = "1", UIMin = "0.1", UIMax = "1", HideEditConditionToggle, EditCondition =
			"RelevancyTargetType != ETraceTarget::ActorLocation"))
	float TargetsRequired = 0.5f;

	//Quality for Visibility  Check
	UPROPERTY(EditDefaultsOnly, Category="LXR|Detection|Visibility")
	ECheckQuality VisibilityCheckQuality = ECheckQuality::Medium;

	//Method to use for tracing checks.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Detection|Visibility")
	ETaskProcessType VisibilityCheckType = ETaskProcessType::Multithread;

	//Target type to use for detection.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Detection|Visibility")
	ETraceTarget VisibilityTargetType = ETraceTarget::ActorLocation;

	//Rate for checking if light is visible to Actor.
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Visibility|Quality", meta=(HideEditConditionToggle, EditCondition = "RelevantCheckQuality == ECheckQuality::Custom"))
	float VisibilityLightCheckRate = 0.05;

	//How many relevant lights we process per check.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Detection|Visibility")
	int VisibilityLightBatchCount = 50;

	//Ignore Self on visibility traces.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXR|Detection|Visibility")
	bool bIgnoreSelfOnVisibilityTraces = true;

	//Trace channel to use for traces.
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Visibility")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	//Percentage of how many traces needs to be passed (not hit) to be lit. 
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Visibility",
		meta=(DisplayName="Traces Required", ClampMin = "0.1", ClampMax = "1", UIMin = "0.1", UIMax = "1", HideEditConditionToggle, EditCondition =
			"RelevantTargetType != ETraceTarget::ActorLocation"))
	float TracesRequired = 0.5f;

	//How many  times in row relevant check must fail to remove light from relevant list. 
	UPROPERTY(EditAnywhere, Category="LXR|Detection|Visibility")
	float MaxConsecutiveFails = 5;

	// UPROPERTY(BlueprintAssignable, Category="LXR|Detection|Visibility")
	// FOnLightCheckChanged OnLightCheckChanged;

	// Combined LXR color from all received LXR Sources  
	UPROPERTY(BlueprintReadOnly, Category="LXR|Detection|Passed")
	FLinearColor CombinedLXRColor;

	//Combined LXR intensity from all received LXR Sources
	UPROPERTY(BlueprintReadOnly, Category="LXR|Detection|Passed")
	float CombinedLXRIntensity;

	// //List of all illuminated targets
	UFUNCTION(BlueprintCallable, Category="LXR|Detection|Passed")
	TMap<int, FLXR> GetIlluminatedTargets();


	//List of actors to ignore when checking visibility.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Detection")
	TArray<TObjectPtr<AActor>> IgnoreVisibilityActors;

	UFUNCTION()
	void OnRep_Replicated_DetectedLightIntensity();
	UPROPERTY(ReplicatedUsing=OnRep_Replicated_DetectedLightIntensity, BlueprintReadOnly, Category="LXR|Detection|Networking")
	float Replicated_DetectedLightIntensity;

	UFUNCTION()
	void OnRep_Replicated_DetectedLightColor();
	UPROPERTY(ReplicatedUsing=OnRep_Replicated_DetectedLightColor, BlueprintReadOnly, Category="LXR|Detection|Networking")
	FLinearColor Replicated_DetectedLightColor;


	/**
	* Get relevant trace type targets of this detection component.
	* @return Relevant Trace Type Targets
	*/
	UFUNCTION(BlueprintPure, Category="LXR|Detection|TraceTargets")
	TArray<FVector> GetVisibilityTraceTypeTargets() const;

	/**
	 * Get all lights which has passed detection test.
	 * @return Passed lights as Array of Actors
	 */
	UFUNCTION(BlueprintPure, Category="LXR|Detection|PassedLights")
	TArray<ULXRSourceComponent*> GetPassedLights() const;

	/**
	 * Get all light components of a light which has passed detection test.
	 * If LightSourceOwner is not in passed list return empty array
	 *
	 * @param LightSourceOwner Passed light actor to get components from.
	 * @return Components of passed light.
	 */
	UFUNCTION(BlueprintPure, Category="LXR|Detection|PassedLights")
	TArray<ULightComponent*> GetPassedLightComponents(ULXRSourceComponent* LightSourceOwner);

	UFUNCTION(BlueprintPure, Category="LXR|Detection|IlluminatedTargets")
	int GetTraceTargetIndexBySocketName(FName SocketName) const;

	UFUNCTION(BlueprintPure, Category="LXR|Detection|IlluminatedTargets")
	FName GetSocketNameByTraceTargetIndex(int TraceTargetIndex);

	TArray<FVector> GetTraceTargets(const bool& bIsRelevant, const ETraceTarget TargetOverride = ETraceTarget::None) const;

	// bool GetIsRelevant(ULXRSourceComponent& LightSourceComponent) const;

	FORCEINLINE void ForceUpdateOctreeLights() { bUpdateOctreeLights = true; }

	//Broadcasts when this Detection component is seen by silhouette component.
	//Blueprint Assignable event
	//@return Detector : Detector Actor with silhouette component. 
	UPROPERTY(BlueprintAssignable, Category="LXR|Detection|Silhouette")
	FOnSeenBySilhouette OnSeenBySilhouette;

	// ELightArrayType GetSmartArrayTypeForLight(const FVector& Start, const FVector& End) const;
	// ELightArrayType GetSmartArrayTypeForLightFromSqrDistance(const float& SqrDist) const;

	
	ELightBucket GetSmartBucketForLight(const FVector& Start, const FVector& End) const;
	ELightBucket GetSmartBucketForLightFromSqrDistance(const float& SqrDist) const;

	// void AddToSmartArrayBySmartArrayType(ELightArrayType LightArrayType, ULXRSourceComponent& SourceComponent);
	

	// Uses Sockets in some manner.
	UFUNCTION(BlueprintCallable, Category="LXR|Detection|Sockets")
	FORCEINLINE bool IsSocketsEnabled() const { return RelevancyTargetType == ETraceTarget::Sockets || VisibilityTargetType == ETraceTarget::Sockets; }

int GetLightToIndex(const TWeakObjectPtr<ULXRSourceComponent> SourcceComponent);
	
private:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void GetLightSystemLights();
	void AddLight(ULXRSourceComponent* LightSource);
	void RemoveLight(ULXRSourceComponent* LightSource);
	void CheckAllLightForRelevancy();
	void UpdateOctreeLights();
	// void AddNewLights();
	// bool AddNewLightToRelevantList(int EntryIndex);
	// void RemoveRedundantLights();
	// void RemoveAllStaleLights();
	void RemoveStaleLights();
	void MoveEntryToBucket(const ELightBucket& NewBucket, int Index);
	void RemoveRelevantLight(int EntryIndex);
	void AddRelevantLight(int EntryIndex);
	void GetLXR();
	// void GetNextBatchByLightArrayType(TArray<TWeakObjectPtr<ULXRSourceComponent>>& OutLightBatch, ELightArrayType LightArrayType);
	// void GetNextBatchByLightArrayType(TArray<TWeakObjectPtr<ULXRSourceComponent>>& OutLightBatch, ELightArrayType LightArrayType);
	void GetNextBucketBatch(TArray<int32>& OutLightBatch, ELightBucket LightBucketType);

	// void SetCurrentLightArrayIndexByLightArrayType(int InIndex, const ELightArrayType LightArrayType);
	 void SetCurrentLightArrayIndexByLightBucketType(int InIndex, const ELightBucket LightBucketType);
	// void ProcessRelevancyCheckLightBatch(TArray<TWeakObjectPtr<ULXRSourceComponent>>& LightBatch, ELightArrayType LightArrayType);
	void ProcessRelevancyCheckLightBucketBatch(TArray<int32>& OutLightBatch, ELightBucket LightBucket);
	void ProcessSmartEntry(int32 EntryIndex,ELightBucket CurrentBucket,float DebugLifetime,const FColor& DebugColor);
	ULXRSourceComponent* GetValidLight(int32 EntryIndex, FLightEntry*& OutEntry);
	float GetDistSqrToOwner(const ULXRSourceComponent* Light) const;
	bool IsInsideSmartMaxRangeSqr(float DistSqr, const ULXRSourceComponent* Light) const;
	void DrawRelevancyDebug(const ULXRSourceComponent* Light, float Lifetime, const FColor& Color) const;
	void ProcessPendingEntries();

	bool CheckDistance(const TWeakObjectPtr<ULXRSourceComponent> LightSourceComponent) const;

	// int GetCurrentLightArrayIndexByLightArrayType(const ELightArrayType LightArrayType) const;
	int GetCurrentBucketIndexByBucketType(const ELightBucket LightBucket) const;
	void AssignPendingToValidBucket(int EntryIndex);

	// TArray<TWeakObjectPtr<ULXRSourceComponent>>& GetLightArrayByLightArrayType(ELightArrayType LightArrayType);


	FCollisionQueryParams GetCollisionQueryParams(const TArray<AActor*>& ActorsToIgnore) const;

	UPROPERTY()
	TObjectPtr<ULXRMemoryComponent> MemoryComponent;

	UPROPERTY()
	TObjectPtr<ULXRSubsystem> LXRSubsystem;

	bool bStop = false;
	bool bUpdateOctreeLights = false;

	bool AsyncEnabledAtStart;

	int SmartFarLightIndex = 0;
	int SmartMidLightIndex = 0;
	int SmartNearLightIndex = 0;
	int RelevancyLightIndex = 0;
	int RelevantLightIndex = 0;
	int AllLightSourceLightActorComponentIndex = 0;
	int RelevantLightSourceActorLightComponentIndex = 0;

	float NearSmartTimer = 0;
	float MidSmartTimer = 0;
	float FarSmartTimer = 0;
	float StatResetTimer = 0;
	float GetCombinedDatasTimer = 0;
	int DebugOctreeCounter = 10;

	mutable FRWLock RelevantDataLockObject;

	FTimerHandle CheckAllLightsTimerHandle;
	FTimerHandle CheckRelevantLightsTimerHandle;
	FBoxCenterAndExtent OctreeBoundsTestObject;

	FVector LastRelevancyUpdateLocation;

	TArray<FLightEntry> Entries;
	TMap<TWeakObjectPtr<ULXRSourceComponent>, int32> LightToIndex;
	TArray<int32> Pending;
	TArray<int32> Buckets[5];
	TArray<int32>& SmartNearBucket = Buckets[static_cast<int>(ELightBucket::SmartNear)];
	TArray<int32>& SmartMidBucket = Buckets[static_cast<int>(ELightBucket::SmartMid)];
	TArray<int32>& SmartFarBucket = Buckets[static_cast<int>(ELightBucket::SmartFar)];
	TArray<int32>& OctreeBucket = Buckets[static_cast<int>(ELightBucket::Octree)];
	TArray<int32>& RelevantBucket = Buckets[static_cast<int>(ELightBucket::Relevant)];

	TMap<int32,int32> OctreeRelevancyChecker;
	bool GetBucketByLightBucketType(ELightBucket LightBucketType, TArray<int32>& OutBucket);

	
	
	TArray<int32> FreeSlots; //if a light is removed during runtime, this array keeps list of valid indices which could be filled with new entries 

void SwapRemoveFromBucket(int EntryIndex);
int AllocateEntry(ULXRSourceComponent* Light);
	void FreeEntry(int EntryIndex);
	// UPROPERTY()
	// TArray<TWeakObjectPtr<ULXRSourceComponent>> OctreeLightsToRemove;
	// UPROPERTY()
	// TArray<TWeakObjectPtr<ULXRSourceComponent>> OctreeLightsToAdd;
	// UPROPERTY()
	// TArray<TWeakObjectPtr<ULXRSourceComponent>> RelevantOctreeLights;
	// UPROPERTY()
	// TArray<TWeakObjectPtr<ULXRSourceComponent>> NearbyOctreeLights;
	// UPROPERTY()
	// TArray<TWeakObjectPtr<ULXRSourceComponent>> SmartNearLights;
	// UPROPERTY()
	// TArray<TWeakObjectPtr<ULXRSourceComponent>> SmartMidLights;
	// UPROPERTY()
	// TArray<TWeakObjectPtr<ULXRSourceComponent>> SmartFarLights;
	// UPROPERTY()
	// TArray<TWeakObjectPtr<ULXRSourceComponent>> NewAllLightsToAdd;
	// UPROPERTY()
	// TArray<TWeakObjectPtr<ULXRSourceComponent>> LightsToRemove;
	// UPROPERTY()
	// TArray<TWeakObjectPtr<ULXRSourceComponent>> SmartFarLightsToRemove;
	// UPROPERTY()
	// TArray<TWeakObjectPtr<ULXRSourceComponent>> SmartMidLightsToRemove;
	// UPROPERTY()
	// TArray<TWeakObjectPtr<ULXRSourceComponent>> SmartNearLightsToRemove;
	// UPROPERTY()
	// TArray<TWeakObjectPtr<ULXRSourceComponent>> SmartFarLightsToAdd;
	// UPROPERTY()
	// TArray<TWeakObjectPtr<ULXRSourceComponent>> SmartMidLightsToAdd;
	// UPROPERTY()
	// TArray<TWeakObjectPtr<ULXRSourceComponent>> SmartNearLightsToAdd;

	FORCEINLINE float GetSocketWeight(const FName& SocketName) const
	{
		if (const float* FoundWeight = TargetSockets.Find(SocketName))
		{
			return *FoundWeight;
		}
		return 1.0f;
	}

private:
	void SetParamsByQualityLevels();

	void ProcessDetectionTask();
	void DetectionTaskFinished();

	void Client_InterpolateCombinedIntensityTowardsReplicatedIntensity();
	void Client_InterpolateCombinedColorTowardsReplicatedColor();

	TSharedPtr<FLightSourcePassedData> RelevancyPassedData;
	TSharedPtr<FTraceTaskData> RelevancyTaskData;
	TSharedPtr<FDetectionTaskData> DetectionTaskData;

	FLinearColor CombinedLXRColorTarget;

	float CombinedLXRIntensityTarget;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> RealSkeletalMeshComponent;

	TArray<FName> SocketNameToTraceTarget;
	TMap<FName, int> SocketNameToTraceTargetIndex;

	

	// MannequinSockets
	inline static const TMap<FName, float> UE4_Mannequin_Sockets = {
		{FName("head"), 2.0f},
		{FName("spine_02"), 3.0f},
		{FName("lowerarm_l"), 1.0f},
		{FName("lowerarm_r"), 1.0f},
		{FName("upperarm_l"), 1.5f},
		{FName("upperarm_r"), 1.5f},
		{FName("hand_l"), 0.5f},
		{FName("hand_r"), 0.5f},
		{FName("thigh_l"), 1.5f},
		{FName("thigh_r"), 1.5f},
		{FName("calf_l"), 1.0f},
		{FName("calf_r"), 1.0f},
		{FName("foot_l"), 0.5f},
		{FName("foot_r"), 0.5f},
	};

	inline static const TMap<FName, float> UE5_Mannequin_Sockets = {
		{FName("head"), 2.0f},
		{FName("spine_02"), 3.0f},
		{FName("lowerarm_l"), 1.0f},
		{FName("lowerarm_r"), 1.0f},
		{FName("upperarm_l"), 1.5f},
		{FName("upperarm_r"), 1.5f},
		{FName("hand_l"), 0.5f},
		{FName("hand_r"), 0.5f},
		{FName("thigh_l"), 1.5f},
		{FName("thigh_r"), 1.5f},
		{FName("calf_l"), 1.0f},
		{FName("calf_r"), 1.0f},
		{FName("foot_l"), 0.5f},
		{FName("foot_r"), 0.5f},
	};

	FORCEINLINE const TMap<FName, float>* GetPresetSockets(ESocketPreset Preset)
	{
		switch (Preset)
		{
		case ESocketPreset::UE4_Mannequin:
			return &UE4_Mannequin_Sockets;
			break;
		case ESocketPreset::UE5_Mannequin:
			return &UE5_Mannequin_Sockets;
			break;
		default: ;
		}
		return nullptr;
	}
	
	void DisplayDebug(AHUD* HUD, UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& YL, float& YPos);
	
	// Vibe Coded debugs
public:
	UFUNCTION(BlueprintCallable, Category="LXR|Debug")
	void DebugPrintLightRegistryMemory() const;

private:
	static FString FormatBytes(int64 Bytes);
	static void AppendArrayStat(FString& Out, const TCHAR* Name, const TArray<int32>& Arr);
	static void AppendEntryArrayStat(FString& Out, const TCHAR* Name, const TArray<FLightEntry>& Arr);
	static void AppendMapStat(FString& Out, const TCHAR* Name, const TMap<TWeakObjectPtr<ULXRSourceComponent>, int32>& Map);
};
