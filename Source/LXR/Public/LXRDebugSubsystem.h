// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "LXR_Shared.h"
#include "Subsystems/WorldSubsystem.h"
#include "Widget/LXRDebugCanvasWidget.h"
#include "Widget/Data/LXRDebugEntryObject.h"
#include "LXRDebugSubsystem.generated.h"

UENUM(BlueprintType)
enum class ELXRDebugFeature : uint8
{
	None UMETA(DisplayName = "None, not valid."),

	Direct UMETA(DisplayName = "Direct Debug (Detection Component)"),
	Sense UMETA(DisplayName = "Sense Debug (Sense Component"),
	Capture UMETA(DisplayName = "Capture Debug (Flux Component"),
	Sockets UMETA(DisplayName = "Socket Debug (Detection Component && RelevantTargetType == Sockets"),
};

UENUM(BlueprintType)
enum class ELXRDebugComponent : uint8
{
	Detection UMETA(DisplayName = "Detection Component"),
	Sense UMETA(DisplayName = "Sense Component"),
};

USTRUCT(Blueprintable)
struct FLXRDebugFeatures
{
	GENERATED_BODY()
	/**
	 *  Auto-detect features which should be enabled.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Debug|Feature")
	bool bAutoDetect = true;

	/**
	 * Enable Direct Debug
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Debug|Feature")
	bool bEnableDirect = false;
	/**
	 * Enable Sense Debug
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Debug|Feature")
	bool bEnableSense = false;
	/**
	 * Enable Capture Debug
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Debug|Feature")
	bool bEnableCapture = false;
	/**
	 * Enable SocketDebug
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Debug|Feature")
	bool bEnableSockets = false;


	/**
	 * Not Implemented yet
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Debug|Feature|Detection")
	FQueryLXRDebugOptions DetectionComponentDebugOptions;

	/**
	 * Not Implemented yet
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Debug|Feature|Sense")
	FQueryLXRDebugOptions SenseComponentDebugOptions;

	UPROPERTY()
	TObjectPtr<AActor> DebugActor;

	FLinearColor DebugColor;
	FOnDebugFeaturesChanged OnDebugFeaturesChanged;


	bool operator==(const FLXRDebugFeatures& Other) const
	{
		return bEnableDirect == Other.bEnableDirect && bEnableSense == Other.bEnableSense && bEnableCapture == Other.bEnableCapture && bEnableSockets == Other.bEnableSockets;
	}

	void UpdateComponentDebugSettings(ELXRDebugComponent ComponentToUpdate, const FQueryLXRDebugOptions& NewDebugOptions) const;
	void UpdateDebugFeatures(const FLXRDebugFeatures& Other);
};

DECLARE_DELEGATE_OneParam(FOnDebugActorAdded, AActor*);
DECLARE_DELEGATE_OneParam(FOnDebugActorRemoved, AActor*);
/**
 *  This subsystem makes it easy to debug add any number of actors with LXR components.
 *  use function AddDebugActor() to request a new debug widget for the actor.
 *  use function RemoveDebugActor() to remove widget created for the actor.
 */
UCLASS(Blueprintable)
class LXR_API ULXRDebugSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	ULXRDebugSubsystem(const FObjectInitializer&);

	FLinearColor GetDebugColor();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;


	/**
	 * Add new Debug Actor
	 * @param InActor Actor To Add
	 * @param InDebugFeatures Features to enable.
	 * @param IsLocalControllerActor Can be left default if APawn class is used.
	*/
	UFUNCTION(BlueprintCallable, Category="LXR|Debug")
	void AddDebugActor(AActor* InActor, const FLXRDebugFeatures& InDebugFeatures, bool IsLocalControllerActor = false);

	/**
	* Update debug features for actor
	* @param InActor Actor to modify debug features
	* @param InDebugFeatures new features.
	*/
	UFUNCTION(BlueprintCallable, Category="LXR|Debug")
	void UpdateActorDebugFeatures(AActor* InActor, const FLXRDebugFeatures& InDebugFeatures);

	/**
	 * Remove added debug actor
	 * @param InActor Actor to remove
	 * @param IsLocalControllerActor Can be left default if APawn class is used.
	 */
	UFUNCTION(BlueprintCallable, Category="LXR|Debug")
	void RemoveDebugActor(AActor* InActor, bool IsLocalControllerActor = false);

	UFUNCTION(BlueprintPure, Category="LXR|Debug")
	int GetAmountOfDebugActors() { return DebugEntries.Num(); }

private:
	int DebugActorCounter = 0;

	TArray<TObjectPtr<ULXRDebugEntryObject>> DebugEntries;

	UPROPERTY()
	TObjectPtr<UULXRDebugCanvasWidget> CanvasWidget;

	/** Class of Blueprint Debug Entry*/
	TSubclassOf<UULXRDebugEntry> DebugEntryWidgetClass;

	/** Class of Blueprint Debug Canvas*/
	TSubclassOf<UULXRDebugCanvasWidget> DebugCanvasWidgetClass;

	ULXRDebugEntryObject* FindEntryByActor(const AActor* InActor);

	void DisplayDebug(AHUD* HUD, UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& YL, float& YPos);

};
