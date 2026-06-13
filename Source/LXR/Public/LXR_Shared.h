// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "Engine/EngineTypes.h"
#include "LXR_Shared.generated.h"

class ULXRSourceComponent;
class ULXRDetectionComponent;
DECLARE_DELEGATE(FOnDebugFeaturesChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSeenBySilhouette, AActor*, Detector);

// enum class ELightArrayType : uint8
// {
// 	All,
// 	SmartFar,
// 	SmartMid,
// 	SmartNear,
// 	Octree,
// 	Relevant
// };

UENUM(BlueprintType)
enum class EMannequinLimb : uint8
{
	Head,
	Hand_R,
	Hand_L,
	LowerArm_R,
	LowerArm_L,
	UpperArm_R,
	UpperArm_L,
	Thigh_R,
	Thigh_L,
	Calf_R,
	Calf_L,
	Foot_R,
	Foot_L,
	Chest
};


UENUM(BlueprintType)
enum class ETraceTarget : uint8
{
	None UMETA(HIDDEN),
	// Owner Location.
	ActorLocation UMETA(DisplayName = "Actor Location"),
	// Bones or sockets to use from skeletal mesh.
	Sockets UMETA(DisplayName = "SkeletalMesh Sockets"),
	// TargetVectors property to use as targets.
	VectorArray UMETA(DisplayName = "Vector Array"),
	// six vectors from actor bounds. (Approximate of actor size) 
	ActorBounds UMETA(DisplayName = "Actor Bounds"),
};

UENUM(BlueprintType)
enum class ESocketPreset : uint8
{
	UE4_Mannequin UMETA(DisplayName = "Unreal Engine 4 Mannequin"),
	UE5_Mannequin UMETA(DisplayName = "Unreal Engine 5 Mannequin"),
	Custom UMETA(DisplayName = "Custom"),
};

UENUM()
enum class ETraceTransform
{
	None UMETA(HIDDEN),
	// AI Pawn Location.
	Actor UMETA(DisplayName = "Actor location and forward"),
	// Actor Eyes.
	ActorEyesViewPoint UMETA(DisplayName = "Use Pawn GetActorEyesViewPoint method, can be overriden in C++"),
	// Use Mesh Socket
	Socket,
	// Custom.
	Custom UMETA(DisplayName = "Implement ILXRSense GetLightSenseTraceLocationAndDirection"),
	// Method Object.
	MethodObject UMETA(DisplayName = "Use LXR Method Object"),
};

UENUM()
enum class ECheckQuality
{
	Low,
	Medium,
	High,
	Epic,
	Custom,
};


USTRUCT(BlueprintType)
struct LXR_API FLightSourceData
{
	GENERATED_USTRUCT_BODY()



	UPROPERTY(EditAnywhere, Category="LXR|Source", meta=(UseComponentPicker, AllowedClasses="/Script/Engine.LightComponent"))
	FComponentReference LightComponent;

	UPROPERTY(EditAnywhere, Category="LXR|Source")
	float LightData;
};

USTRUCT(BlueprintType)
struct LXR_API FQueryLXRDebugOptions
{
	GENERATED_USTRUCT_BODY()
	FQueryLXRDebugOptions(): bPrintDebug(false), bDebugRelevancy(false), bDebugTargets(false), bDebugVisibility(false), bDebugLXR(false), bDebugLXR_OnlyPassed(false)
	{
	};

	void SetAllDrawTimes(float InDrawTime)
	{
		DebugRelevancyDrawTime = InDrawTime;
		DebugVisibilityDrawTime = InDrawTime;
		DebugLXRDrawTime = InDrawTime;
	}

	/// Print debugs to console.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Debug")
	bool bPrintDebug;

	/// Draw any debugs related to relevancy step
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Debug")
	bool bDebugRelevancy;

	/// Draw any debugs related to targets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Debug")
	bool bDebugTargets;

	/// Draw any debugs related to visibility step
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Debug")
	bool bDebugVisibility;

	/// Draw any debugs related to the final Lux
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Debug")
	bool bDebugLXR;

	/// Draw any debugs related to the final Lux, only passed targets.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Debug")
	bool bDebugLXR_OnlyPassed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Debug")
	float DebugRelevancyDrawTime = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Debug")
	float DebugVisibilityDrawTime = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Debug")
	float DebugLXRDrawTime = 1.f;
};

UENUM(BlueprintType)
enum class EDetectionType : uint8
{
	Detection,
	LightSense,
	Silhouette,
	Environment
};


UENUM(BlueprintType)
enum class ETaskProcessType : uint8
{
	// Use Synchronous LineTrace for relevant light visibility checks
	Sync UMETA(DisplayName = "Synchronous LineTrace"),
	// Use separate thread for relevant traces
	Multithread UMETA(DisplayName = "Multithread")
};

/*
Data about passed light sources, light components and targets.
Not really usable in Blueprint.
*/
USTRUCT(BlueprintType)
struct LXR_API FLightSourcePassedData
{
	GENERATED_USTRUCT_BODY()

	FLightSourcePassedData()
	{
	}

	// TWeakObjectPtr<AActor> LightSourceActor;
	TWeakObjectPtr<ULXRSourceComponent> SourceComponent;
	TMap<int, TSet<int>> PassedComponentsAndTargets;
	// float RequiredChecksToPassAmount = 1;

	// FLightSourcePassedData(TWeakObjectPtr<ULXRSourceComponent> InSourceComponent, TWeakObjectPtr<AActor> InSourceLightOwner)
	// {
	// 	SourceComponent = InSourceComponent;
	// 	LightSourceActor = InSourceLightOwner;
	// }
	
	FLightSourcePassedData(TWeakObjectPtr<ULXRSourceComponent> InSourceComponent)
	{
		SourceComponent = InSourceComponent;
	}

	// ULXRSourceComponent* GetSourceComponent() const
	// {
	// 	return LightSourceActor->GetComponentByClass<ULXRSourceComponent>();
	// }

	// FLightSourcePassedData(TWeakObjectPtr<AActor> InLightSourceActor, const TMap<int, TSet<int>>& InPassedComponentsAndTargets)
	// {
	// 	LightSourceActor = InLightSourceActor;
	// 	PassedComponentsAndTargets = InPassedComponentsAndTargets;
	// }

	FLightSourcePassedData(TWeakObjectPtr<ULXRSourceComponent> InSourceComponent, const TMap<int, TSet<int>>& InPassedComponentsAndTargets)
	{
		PassedComponentsAndTargets = InPassedComponentsAndTargets;
	}


	void AddPassedComponent(int Component)
	{
		if (!PassedComponentsAndTargets.Contains(Component))
			PassedComponentsAndTargets.Add(Component);
	}

	void AddPassedComponentsAndTargets(const int Component, const TSet<int>& PassedTargets)
	{
		AddPassedComponent(Component);
		PassedComponentsAndTargets[Component].Append(PassedTargets);
	}

	TSet<int> GetAllPassedTargets() const
	{
		TSet<int> Temp;
		for (auto& Pair : PassedComponentsAndTargets)
		{
			Temp.Append(Pair.Value);
		}
		return Temp;
	}

	TSet<int> GetAllComponents() const
	{
		TSet<int> Temp;
		for (auto& Pair : PassedComponentsAndTargets)
		{
			Temp.Add(Pair.Key);
		}
		return Temp;
	}

	TArray<int> GetAllPassedComponentsAsArray() const
	{
		TArray<int> Temp;
		for (auto& Pair : PassedComponentsAndTargets)
		{
			Temp.Add(Pair.Key);
		}
		return Temp;
	}

	void Clear()
	{
		PassedComponentsAndTargets.Empty();
	}

	bool IsValid() const
	{
		return SourceComponent.IsValid() && PassedComponentsAndTargets.Num() > 0;
	}

	// void RemovePassedComponentAndTarget(const int Component, const int TargetToRemove)
	// {
	// 	if (PassedComponentsAndTargets[Component].Contains(TargetToRemove))
	// 		PassedComponentsAndTargets[Component].Remove(TargetToRemove);
	// }

	void RemovePassedTargetForComponent(const int Component, const int TargetToRemove)
	{
		if (PassedComponentsAndTargets[Component].Contains(TargetToRemove))
			PassedComponentsAndTargets[Component].Remove(TargetToRemove);
	}

	void RemovePassedTargetsForComponent(const int Component, const TSet<int>& TargetsToRemove)
	{
		if (PassedComponentsAndTargets.Contains(Component))
		{
			for (const int TargetToRemove : TargetsToRemove)
			{
				RemovePassedTargetForComponent(Component, TargetToRemove);
			}
		}
	}

	void AddPassedComponentAndTarget(const int Component, const int Target)
	{
		if (!PassedComponentsAndTargets.Contains(Component))
			PassedComponentsAndTargets.Add(Component);

		PassedComponentsAndTargets[Component].Add(Target);
	}

	bool operator==(const FLightSourcePassedData& Other) const
	{
		return SourceComponent == Other.SourceComponent;
	}

	bool operator==(const TWeakObjectPtr<ULXRSourceComponent>& InSourceComponent) const
	{
		return InSourceComponent == SourceComponent;
	}
};


USTRUCT(BlueprintType)
struct LXR_API FLXR
{
	GENERATED_USTRUCT_BODY()

	FLXR(): Intensity(0), Color()
	{
		Intensity = 0;
		WeightedIntensity = 0;
		Color = FLinearColor::Black;
	};
	FLXR(float InIntensity, FLinearColor InColor): WeightedIntensity(0) { Intensity = InIntensity, Color = InColor; } ;

	UPROPERTY(BlueprintReadOnly, Category="LXR|Final LXR")
	float AverageIntensity;

	UPROPERTY(BlueprintReadOnly, Category="LXR|Final LXR")
	float Intensity;

	UPROPERTY(BlueprintReadOnly, Category="LXR|Final LXR")
	float WeightedIntensity;

	UPROPERTY(BlueprintReadOnly, Category="LXR|Final LXR")
	FLinearColor Color;

	bool operator==(const FLXR& Other) const
	{
		auto A = Color * Intensity;
		auto B = Other.Color * Other.Intensity;
		return A == B;
	}
};

/*
 Data about the final LXR
 */
USTRUCT(BlueprintType)
struct LXR_API FTargetLXRData
{
	GENERATED_USTRUCT_BODY()

	FTargetLXRData(): IlluminatedTargetsCount(0)
	{
	}

	//ALL Trace Targets of this trace batch, not only passed ones. 
	UPROPERTY(BlueprintReadOnly, Category="LXR|Final LXR")
	TArray<FVector> TraceTargets;
	// LXR amount of targets, key of map is index of TraceTargets array. Contains only passed targets.
	UPROPERTY(BlueprintReadOnly, Category="LXR|Final LXR")
	TMap<int, FLXR> TargetsLXR;
	// Amount of illuminated targets
	UPROPERTY(BlueprintReadOnly, Category="LXR|Final LXR")
	int IlluminatedTargetsCount;

	void Reset()
	{
		TraceTargets.Reset();
		TargetsLXR.Reset();
		IlluminatedTargetsCount = 0;
	}

	void AddTarget(int Target)
	{
		if (!TargetsLXR.Contains(Target))
			TargetsLXR.Add(Target);
	}

	void AddTargetAndValues(const int Target, float InIntensity, FLinearColor InColor) { TargetsLXR.Add(Target, {InIntensity, InColor}); }
	void AddTargetLXR(const int Target, FLXR InLXR) { TargetsLXR.Add(Target, InLXR); }

	bool operator==(const FTargetLXRData& Other) const
	{
		return TraceTargets == Other.TraceTargets;
	}

	TArray<float> GetAllTargetsLuxValues() const
	{
		TArray<float> OutLuxes;
		for (auto& Pair : TargetsLXR)
		{
			OutLuxes.Add(Pair.Value.Intensity);
		}
		return OutLuxes;
	}

	TArray<float> GetAllTargetsAverageLuxValues() const
	{
		TArray<float> OutLuxes;
		for (auto& Pair : TargetsLXR)
		{
			OutLuxes.Add(Pair.Value.AverageIntensity);
		}
		return OutLuxes;
	}
	

	TArray<FLinearColor> GetAllTargetColorsAsArray() const
	{
		TArray<FLinearColor> Colors;
		Colors.Reserve(TargetsLXR.Num());
		for (auto& Pair : TargetsLXR)
		{
			Colors.Add(Pair.Value.Color);
		}
		return Colors;
	}

	FLinearColor GetAverageColor() const;

	float GetWeightedAverageLuxFromAllTargets(float MinLuxThreshold, float WeightToUseAboveMinLuxThreshold = 0.7f) const;
	float GetAverageLuxFromAllTargets() const;
	float GetWeightedAverageLuxFromPassedTargets(float MinLuxThreshold, float WeightToUseAboveMinLuxThreshold = 0.7f) const;
	float GetMaxLux() const;
	float GetAverageLuxFromPassedTargets() const;
	float GetSocketWeightedLux(ULXRDetectionComponent& DetectionComponent);
};

//
// /*
// Data about passed light sources, light components and targets.
// Not really usable in Blueprint.
// */
// USTRUCT(BlueprintType)
// struct LXR_API FLightSourcePassedData_New
// {
// 	GENERATED_USTRUCT_BODY()
//
// 	FLightSourcePassedData_New()
// 	{
// 	}
//
// 	TWeakObjectPtr<AActor> LightSourceActor;
// 	TWeakObjectPtr<ULXRSourceComponent> LXRSourceComponent;
// 	TMap<int, TSet<int>> PassedComponentsAndTargets;
// 	float RequiredChecksToPassAmount = 1;
//
// 	FLightSourcePassedData_New(ULXRSourceComponent* InSourceComponent)
// 	{
// 		LXRSourceComponent = InSourceComponent;
// 		LightSourceActor = LXRSourceComponent->GetOwner();
// 	}
//
// 	void AddPassedComponent(int Component)
// 	{
// 		if (!PassedComponentsAndTargets.Contains(Component))
// 			PassedComponentsAndTargets.Add(Component);
// 	}
//
// 	void AddPassedComponentsAndTargets(const int Component, const TSet<int>& PassedTargets)
// 	{
// 		AddPassedComponent(Component);
// 		PassedComponentsAndTargets[Component].Append(PassedTargets);
// 	}
//
// 	TSet<int> GetAllPassedTargets() const
// 	{
// 		TSet<int> Temp;
// 		for (auto& Pair : PassedComponentsAndTargets)
// 		{
// 			Temp.Append(Pair.Value);
// 		}
// 		return Temp;
// 	}
//
// 	TSet<int> GetAllComponents() const
// 	{
// 		TSet<int> Temp;
// 		for (auto& Pair : PassedComponentsAndTargets)
// 		{
// 			Temp.Add(Pair.Key);
// 		}
// 		return Temp;
// 	}
//
// 	TArray<int> GetAllPassedComponentsAsArray() const
// 	{
// 		TArray<int> Temp;
// 		for (auto& Pair : PassedComponentsAndTargets)
// 		{
// 			Temp.Add(Pair.Key);
// 		}
// 		return Temp;
// 	}
//
// 	void Clear()
// 	{
// 		PassedComponentsAndTargets.Empty();
// 	}
//
// 	bool IsValid() const
// 	{
// 		return LightSourceActor.IsValid() && PassedComponentsAndTargets.Num() > 0;
// 	}
//
// 	// void RemovePassedComponentAndTarget(const int Component, const int TargetToRemove)
// 	// {
// 	// 	if (PassedComponentsAndTargets[Component].Contains(TargetToRemove))
// 	// 		PassedComponentsAndTargets[Component].Remove(TargetToRemove);
// 	// }
//
// 	void RemovePassedTargetForComponent(const int Component, const int TargetToRemove)
// 	{
// 		if (PassedComponentsAndTargets[Component].Contains(TargetToRemove))
// 			PassedComponentsAndTargets[Component].Remove(TargetToRemove);
// 	}
//
// 	void RemovePassedTargetsForComponent(const int Component, const TSet<int>& TargetsToRemove)
// 	{
// 		if (PassedComponentsAndTargets.Contains(Component))
// 		{
// 			for (const int TargetToRemove : TargetsToRemove)
// 			{
// 				RemovePassedTargetForComponent(Component, TargetToRemove);
// 			}
// 		}
// 	}
//
// 	void AddPassedComponentAndTarget(const int Component, const int Target)
// 	{
// 		if (!PassedComponentsAndTargets.Contains(Component))
// 			PassedComponentsAndTargets.Add(Component);
//
// 		PassedComponentsAndTargets[Component].Add(Target);
// 	}
//
// 	bool operator==(const FLightSourcePassedData& Other) const
// 	{
// 		return LightSourceActor == Other.LightSourceActor;
// 	}
//
// 	bool operator==(const TWeakObjectPtr<AActor>& LightActor) const
// 	{
// 		return LightSourceActor == LightActor;
// 	}
// };


FORCEINLINE uint32 GetTypeHash(const FLightSourcePassedData& LightSourcePassedData)
{
	const uint32 Hash = FCrc::MemCrc32(&LightSourcePassedData, sizeof(FLightSourcePassedData));
	return Hash;
}

FORCEINLINE uint32 GetTypeHash(const FLXR& LXR)
{
	const uint32 Hash = FCrc::MemCrc32(&LXR, sizeof(FLXR));
	return Hash;
}

FORCEINLINE uint32 GetTypeHash(const FTargetLXRData& IlluminatedTargets)
{
	const uint32 Hash = FCrc::MemCrc32(&IlluminatedTargets, sizeof(FTargetLXRData));
	return Hash;
}

