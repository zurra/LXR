// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LXR.h"
#include "LXROctree.h"
#include "LXR_Shared.h"
#include "UnrealEngine.h"
#include "Blueprint/UserWidget.h"
#include "LXRSubsystem.generated.h"

DECLARE_EVENT_OneParam(ULightDetectionSubsystem, FOnLightChanged, ULXRSourceComponent*);

// TSet<FLightSourcePassedData>& PassedData, FTargetsLXR& IlluminatedTargets

DECLARE_DWORD_COUNTER_STAT(TEXT("Relevant Lights"), STAT_RELEVANTLIGHTS, STATGROUP_LXR);
DECLARE_DWORD_COUNTER_STAT(TEXT("Passed Relevant Lights"), STAT_PASSEDRELEVANTLIGHTS, STATGROUP_LXR);
DECLARE_DWORD_COUNTER_STAT(TEXT("All Lights"), STAT_ALLLIGHTS, STATGROUP_LXR);
DECLARE_DWORD_COUNTER_STAT(TEXT("Smart Near"), STAT_SMARTNEAR, STATGROUP_LXR);
DECLARE_DWORD_COUNTER_STAT(TEXT("Smart Mid"), STAT_SMARTMID, STATGROUP_LXR);
DECLARE_DWORD_COUNTER_STAT(TEXT("Smart Far"), STAT_SMARTFAR, STATGROUP_LXR);
DECLARE_DWORD_COUNTER_STAT(TEXT("RelevantLights"), STAT_RELEVANT, STATGROUP_LXR);
DECLARE_DWORD_COUNTER_STAT(TEXT("Light Sense"), STAT_LIGHTSENSE, STATGROUP_LXR);

DECLARE_CYCLE_STAT(TEXT("Relevant Check"), STAT_RelevantCheck, STATGROUP_LXR);
DECLARE_CYCLE_STAT(TEXT("Relevancy Check"), STAT_RelevancyCheck, STATGROUP_LXR);
DECLARE_CYCLE_STAT(TEXT("Get Combined Datas"), STAT_GetCombinedDatas, STATGROUP_LXR);
DECLARE_CYCLE_STAT(TEXT("Get Combined Sense Datas"), STAT_GetCombinedSenseDatas, STATGROUP_LXR);
DECLARE_CYCLE_STAT(TEXT("Light Sense Check"), STAT_LightSenseCheck, STATGROUP_LXR);
DECLARE_CYCLE_STAT(TEXT("Light Detection Check"), STAT_LightDetectionCheck, STATGROUP_LXR);
// DECLARE_CYCLE_STAT(TEXT("Indirect Capture"), STAT_IndirectCapture, STATGROUP_LXR);
// DECLARE_CYCLE_STAT(TEXT("Indirect Texture Process"), STAT_IndirectProcess, STATGROUP_LXR);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQueryLXRDone, const FTargetLXRData&, TargetsLXR, const TSet<FLightSourcePassedData>&, PassedData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTraceTargetOperationDone, const TSet<FVector>&, TraceTargets);

/**
 * 
 */
UCLASS()
class LXR_API ULXRSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	FOnLightChanged OnLightAdded;
	FOnLightChanged OnLightRemoved;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	//Registers new light source for LXR
	UFUNCTION(BlueprintCallable, Category="LXR")
	void RegisterLight(ULXRSourceComponent* LightSource);

	virtual void Tick(float DeltaTime) override;

	virtual TStatId GetStatId() const override;

	//Removes light source from LXR
	UFUNCTION(BlueprintCallable, Category="LXR")
	void UnregisterLight(ULXRSourceComponent* LightSource);

	void RegisterDetection(ULXRDetectionComponent* DetectionActor);
	void UnregisterDetection(ULXRDetectionComponent* DetectionActor);

	const TArray<TWeakObjectPtr<ULXRSourceComponent>>& GetAllLights() const;

	void CreateOctree();

	void GetOctreeLights(FBoxCenterAndExtent& OctreeBoundsTestObject, TSet<TWeakObjectPtr<ULXRSourceComponent>>& Lights, bool bDebug = false) const;

	TSharedPtr<FLXROctreeSemantics::FLXROctree, ESPMode::ThreadSafe>
	GetOctree() const { return LXROctreeObject->Octree; }
	
	bool bSoloFound;

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<ULXRSourceComponent>> LightSources;

	UPROPERTY()
	TArray<TWeakObjectPtr<ULXRDetectionComponent>> DetectionComponents;


	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> LXRActorsNotInOctreeYetBuffer;


	// Octree stuff
public:
	void FindLXRPoints(TArray<FLXROctreeElement>& OutElements, const FBoxCenterAndExtent& QueryBox) const;
	void FindLXRPoints(TArray<FLXROctreeElement>& OutElements, const FSphere& QuerySphere) const;
	void AddLXRPoints(const TArray<FDTOLXRData>& LXRDTOs);
	void AddLXRActorToTree(AActor* Actor);
	void RemoveStaleLXRPoints(FBox Area);
	void RemoveStaleLXRPoints(FVector Origin, FVector Extent);
	void RemoveOctreeElement(const AActor* SourceObject);
	void RemoveOctreeElement(const unsigned UID);
	void AssignIDToElement(const AActor* SourceObject, FOctreeElementId2 ID);
	bool GetElementID(FOctreeElementId2& OutElementID, const AActor* SourceObject) const;
	bool GetElementID(FOctreeElementId2& OutElementID, const unsigned UID) const;

	// ULXRMethodObject* GetOrAddMethodObject(const UClass* MethodObjectClass);

	void InformNearbyDetectionActorsToUpdateOctreeLights(const AActor* SourceActor);

	FVector UpdateElement(AActor* SourceObject);

	TTuple<FVector*, FRotator*>* GetPlayerCameraLocationAndRotation();


private:
	bool RemoveIDToElementMapping(const AActor* ElementActor);
	bool RemoveIDToElementMapping(const unsigned UID);

	void UpdateElement(const unsigned UID);
	

	TSharedPtr<FLXROctreeObject, ESPMode::ThreadSafe> LXROctreeObject;


	TMap<uint32, FOctreeElementId2> UIDToElementID;
	TMap<uint32, TWeakObjectPtr<const AActor>> UIDToLXRActor;
	TMap<TWeakObjectPtr<const AActor>, uint32> LXRObjectToUID;

	// TMap<UClass*, ULXRMethodObject*> InUseMethodObjectMap;

	mutable FRWLock LXRTreeLockObject;

	TTuple<FVector*, FRotator*> PlayerCameraLocationAndRotation;

	float PlayerCameraUpdateTimer;

	bool LastFrameDrawDebug = false;
	bool bLXRDetectionDebugWidgetEnabled = true;

	UPROPERTY()
	TWeakObjectPtr<UUserWidget> DetectionDebugWidget;

	FConsoleVariableDelegate DebugWidgetCVarChangedDelegate;
	void OnDebugWidgetCvarChanged();
	
	FConsoleVariableSinkHandle ConsoleHandle;

};
