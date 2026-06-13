// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "Templates/Tuple.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "LXR_Shared.h"
#include "Tasks/TraceTask/Data/FTraceTaskData.h"
#include "Templates/Function.h"
#include "LXRSourceComponent.h"
#include "LXRFunctionLibrary.generated.h"

struct FQueryLXRDebugOptions;
struct FLightSourcePassedData;
struct FSilhouetteTaskData;

UENUM(Blueprintable)
enum class EDominantColor : uint8 { None, Red, Green, Blue };

USTRUCT(BlueprintType)
struct LXR_API FDominantColor
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LXR|Detection")
	EDominantColor Color;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LXR|Detection")
	float ColorValue;

	FDominantColor()
	{
		Color = EDominantColor::None;
		ColorValue = -1;
	}

	float GetColorValueRoundedToHalf() const
	{
		return ColorValue != 0 ? FMath::RoundToFloat(FMath::Min(ColorValue, 1.0f) * 2) / 2 : 0;
	}

	bool operator==(const FDominantColor& Other) const
	{
		return Other.Color == Color;
	}

	uint32 GetTypeHash(const FDominantColor& Other);
};

UCLASS()
class LXR_API ULXRFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
#pragma region v1.1
	// Returns the color channel which has the most influence of InColor.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Dominant Color", CompactNodeTitle = "Dominant Color"), Category= "LXR|Detection|CombinedData")
	static FDominantColor GetDominantColor(const FLinearColor& InColor);

	// Returns the color channel which has the second most influence of InColor.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get 2nd Dominant Color", CompactNodeTitle = "2nd Dominant Color"), Category= "LXR|Detection|CombinedData")
	static FDominantColor GetSecondDominantColor(const FLinearColor& InColor);

	// Clamp the color channels to 0-1 range.
	UFUNCTION(BlueprintPure, meta = (Displayname = "Clamp Channels", CompactNodeTitle = "Clamp", Keywords ="Clamp"), Category= "LXR|Detection|CombinedData")
	static FLinearColor ClampTo01Range(const FLinearColor& InColor);

	// Is DominantColors equal.
	UFUNCTION(BlueprintPure, meta = (Displayname = "Equal DominantColor", CompactNodeTitle = "DomC ==", Keywords =" == equal"), Category= "LXR|Detection|CombinedData")
	static bool Equal_FDominantColor(const FDominantColor& FirstColor, const FDominantColor& SecondColor);

	// Does the LinearColor equal Dominant color.
	UFUNCTION(BlueprintPure, meta = (Displayname = "Color Equal DominantColor", CompactNodeTitle = "Col == DomC", Keywords ="== equal"), Category= "LXR|Detection|CombinedData")
	static bool ColorEqual_DominantColor(const FLinearColor& Color, const FDominantColor& DominantColor);

#pragma endregion

#pragma region v1.2

	// Are the colors approximately equal.
	UFUNCTION(BlueprintPure, meta = (Displayname = "Color Approximately Equal Color", CompactNodeTitle = "~", Keywords ="Approximately ~"), Category= "LXR|Detection|CombinedData")
	static bool ColorApproximatelyEqualColor(const FLinearColor& InColorOne, const FLinearColor& InColorTwo);

	// Are the colors after remapping and rounding to half equal.
	UFUNCTION(BlueprintPure, meta = (Displayname = "Color Rounded Half Equal Color", CompactNodeTitle = "== [½]", Keywords ="[ Equal Half ]"),
		Category= "LXR|Detection|CombinedData")
	static bool ColorRemappedRoundedHalfEqualColor(const FLinearColor& InColorOne, const FLinearColor& InColorTwo);

	// Toggles Color channels according to ToggleChannels.
	UFUNCTION(BlueprintPure, meta = (Displayname = "Toggle Channels"), Category= "LXR|Color")
	static FLinearColor ToggleColorChannels(const FLinearColor& InColor, const FLinearColor& ToggleChannels);

	// Remaps color to 0-1.
	UFUNCTION(BlueprintPure, meta = (Displayname = "Remap Color Range to 0-1", CompactNodeTitle = "|0-1|", Keywords ="Remap Color Range"), Category= "LXR|Detection|CombinedData")
	static FLinearColor RemapColorRangeTo01(const FLinearColor& InColor);

	// Rounds color to nearest half.
	UFUNCTION(BlueprintPure, meta = (Displayname = "Round Channels To Nearest Half", CompactNodeTitle = "[½]", Keywords ="Round Half"), Category= "LXR|Detection|CombinedData")
	static FLinearColor RoundToNearestHalf(const FLinearColor& InColor);

	// Return the max value of color channels.
	UFUNCTION(BlueprintPure, meta = (Displayname = "Get Max of Color Channels", CompactNodeTitle = "Max", Keywords ="Max Color Channel"), Category= "LXR|Detection|CombinedData")
	static float GetMaxOfColorChannels(const FLinearColor& InColor);

	// Return the min value of color channels.
	UFUNCTION(BlueprintPure, meta = (Displayname = "Get Min of Color Channels", CompactNodeTitle = "Min", Keywords ="Max Color Channel"), Category= "LXR|Detection|CombinedData")
	static float GetMinOfColorChannels(const FLinearColor& InColor);

	// Return inverse of LinearColor channels.
	UFUNCTION(BlueprintPure, meta = (Displayname = "Inverse Channels", CompactNodeTitle = "Inverse", Keywords ="Inverse"), Category= "LXR|Detection|CombinedData")
	static FLinearColor GetInverseChannels(const FLinearColor& InColor);

	// Return DominantColor converted to LinearColor.
	UFUNCTION(BlueprintPure, meta = (Displayname = "Dominant Color to Linear Color", CompactNodeTitle = "Dom to Linear", Keywords ="Dominant to Linear"),
		Category= "LXR|Detection|CombinedData")
	static FLinearColor DominantToLinearColor(const FDominantColor& InDominantColor);

#pragma endregion

#pragma region v1.3

	// Get FLinearColor array Average.
	UFUNCTION(BlueprintPure, meta = (Displayname = "Linear Color Array Average", CompactNodeTitle = "Average Color", Keywords ="Linear Color Array Average"),
		Category= "LXR|Detection|CombinedData")
	static FLinearColor GetLinearColorArrayAverage(const TArray<FLinearColor>& InColors);

	// Rounds color channels.
	UFUNCTION(BlueprintPure, meta = (Displayname = "Round Channels", CompactNodeTitle = "[nint]", Keywords ="Round "), Category= "LXR|Detection|CombinedData")
	static FLinearColor Round(const FLinearColor& InColor);

#pragma endregion


	///
	///Copy Paste from UE Wiki
	///Float as String With Precision
	static FORCEINLINE FString GetFloatAsStringWithPrecision(float TheFloat, int32 Precision, bool IncludeLeadingZero = true)
	{
		//Round to integral if have something like 1.9999 within precision
		float Rounded = roundf(TheFloat);
		if (FMath::Abs(TheFloat - Rounded) < FMath::Pow(static_cast<float>(10), static_cast<float>(-1 * Precision)))
		{
			TheFloat = Rounded;
		}
		FNumberFormattingOptions NumberFormat; //Text.h
		NumberFormat.MinimumIntegralDigits = (IncludeLeadingZero) ? 1 : 0;
		NumberFormat.MaximumIntegralDigits = 10000;
		NumberFormat.MinimumFractionalDigits = Precision;
		NumberFormat.MaximumFractionalDigits = Precision;
		return FText::AsNumber(TheFloat, &NumberFormat).ToString();
	}

	//Float as FText With Precision!
	static FORCEINLINE FText GetFloatAsTextWithPrecision(float TheFloat, int32 Precision, bool IncludeLeadingZero = true)
	{
		//Round to integral if have something like 1.9999 within precision
		float Rounded = roundf(TheFloat);
		if (FMath::Abs(TheFloat - Rounded) < FMath::Pow(static_cast<float>(10), static_cast<float>(-1 * Precision)))
		{
			TheFloat = Rounded;
		}
		FNumberFormattingOptions NumberFormat; //Text.h
		NumberFormat.MinimumIntegralDigits = (IncludeLeadingZero) ? 1 : 0;
		NumberFormat.MaximumIntegralDigits = 10000;
		NumberFormat.MinimumFractionalDigits = Precision;
		NumberFormat.MaximumFractionalDigits = Precision;
		return FText::AsNumber(TheFloat, &NumberFormat);
	}


	UFUNCTION(meta = (DisplayName = "Check Trace Target Visibility", WorldContext = "WorldContextObject"), Category= "LXR|Check|Targets")
	static bool CheckVisibility(const UObject* WorldContextObject, const FTraceTaskData& TraceTaskData, FLightSourcePassedData& PassedData,
	                            const FQueryLXRDebugOptions& QueryLXRDebugOptions);

	UFUNCTION(meta = (DisplayName = "Check Trace Target Relevancy", WorldContext = "WorldContextObject"), Category= "LXR|Check|Targets")
	static bool CheckIsLightRelevant(const UObject* WorldContextObject, const FTraceTaskData& TraceTaskData, FLightSourcePassedData& PassedData,
	                                 const FQueryLXRDebugOptions& QueryLXRDebugOptions);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get LUX", WorldContext = "WorldContextObject"), Category= "LXR|GetLXR")
	static bool GetLux(const UObject* WorldContextObject, const FTraceTaskData& TraceTaskData, const TSet<FLightSourcePassedData>& PassedData, struct FTargetLXRData& TargetLXRData,
	                   const struct FQueryLXRDebugOptions& QueryLXRDebugOptions, float LXRMultiplier = 1, float MinLuxThreshold = 10);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get LUX FROM TASK", WorldContext = "WorldContextObject"), Category= "LXR|GetLXR")
	static bool GetLuxFromTask(const UObject* WorldContextObject, const FTraceTaskData& TraceTaskData,
					   const struct FQueryLXRDebugOptions& QueryLXRDebugOptions, float LXRMultiplier = 1, float MinLuxThreshold = 10);

	static void GetRelevantLights(const UObject* WorldContextObject, const TArray<FVector>& TraceTargets, TSet<TWeakObjectPtr<ULXRSourceComponent>>& RelevantLights);
	static void GetLightsInBox(const UObject* WorldContextObject, const FBox& Box, TSet<TWeakObjectPtr<ULXRSourceComponent>>& RelevantLights);

	static void FetchSilhouetteTraceParams(const AActor& FromActor, const AActor& ToActor, FSilhouetteTaskData& SilhouetteTaskData);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Generate Grid Targets", WorldContext = "WorldContextObject"), Category= "LXR|Generate|Targets")
	static TSet<FVector> GenerateGridTargets(const UObject* WorldContextObject, FVector Center, float Radius, float Density);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Generate Cylinder Targets", WorldContext = "WorldContextObject"), Category= "LXR|Generate|Targets")
	static TSet<FVector> GenerateCylinderTargets(const UObject* WorldContextObject, FVector Center, FVector Direction, int CylinderSegments = 6, int CylinderRadius = 100,
	                                             int CylinderDistance = 2000, int DistancePerSegment = 100);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Generate Cone Targets", WorldContext = "WorldContextObject"), Category= "LXR|Generate|Targets")
	static TSet<FVector> GenerateConeTraceTargets(const UObject* WorldContextObject, const FVector& Location, const FVector& Direction, int ConeAngle = 20, int ConeSides = 6,
	                                              int ConeRadius = 100, int ConeDistance = 2000, int DistancePerSegment = 100);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Generate Sphere Targets", WorldContext = "WorldContextObject"), Category= "LXR|Generate|Targets")
	static TSet<FVector> GenerateSphereTraceTargets(FVector const& Center, float Radius, int32 Segments);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Generate Circle Targets", WorldContext = "WorldContextObject"), Category= "LXR|Generate|Targets")
	static TSet<FVector> GenerateCircleTraceTargets(const UObject* WorldContextObject, FVector const& Center, FVector const& Forward, float Radius, int32 Segments);

	// TODO Blueprintable...
	static TSet<FVector> GenerateSilhouetteTraceTargets(const UObject* WorldContextObject, const AActor* FromActor, const AActor* ToActor, FSilhouetteTaskData& SilhouetteTaskDat);


	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Filter Targets By Trace", WorldContext = "WorldContextObject"), Category= "LXR|Filter|Targets")
	static TSet<FVector> FilterTargetsByTrace(const UObject* WorldContextObject, const FVector& FilterTraceStartLocation, const TSet<FVector>& TraceTargets,
	                                          TArray<AActor*> ActorsToIgnore, ECollisionChannel TraceChannel = ECC_Visibility);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Debug LXR", WorldContext = "WorldContextObject"), Category= "LXR|Debug|LXR")
	static void DebugLXR(const UObject* WorldContextObject, const FTargetLXRData& TargetsLXR, float DebugDrawTime, bool bDebugOnlyPassed, float MaxLux);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Debug Targets", WorldContext = "WorldContextObject"), Category= "LXR|Debug|Targets")
	static void DebugTargets(const UObject* WorldContextObject, TSet<FVector> Targets, float DebugDrawTime = 1.f, FLinearColor DebugColor = FLinearColor::Yellow);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Relevant Lights For Trace Targets", WorldContext = "WorldContextObject"), Category= "LXR|RelevantLights")
	static void BP_GetRelevantLights(const UObject* WorldContextObject, const TArray<FVector>& TraceTargets, TSet<ULXRSourceComponent*>& RelevantLights);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Relevant Lights In Box", WorldContext = "WorldContextObject"), Category= "LXR|RelevantLights")
	static void BP_GetLightsInBox(const UObject* WorldContextObject, const FBox& Box, TSet<ULXRSourceComponent*>& RelevantLights);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Relevant Lights In Box (Octree)", WorldContext = "WorldContextObject"), Category= "LXR|RelevantLights")
	static void BP_GetOctreeLights(const UObject* WorldContextObject, const FBox& Box, TSet<ULXRSourceComponent*>& Lights, bool bDebug = false);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Query Locations LXR", WorldContext = "Querier"), Category= "LXR|Query")
	static void QueryLocationsLXR(const UObject* Querier, TSet<FLightSourcePassedData>& PassedData, FTargetLXRData& IlluminatedTargets, TArray<ULXRSourceComponent*> Lights,
	                              const TArray<FVector>& Points, const FQueryLXRDebugOptions& QueryLXRDebugOptions);

	UFUNCTION(BlueprintPure, BlueprintCallable, meta = (DisplayName = "IsPerceivedActorLit", WorldContext = "Querier"), Category= "LXR|Perception")
	static bool IsPerceivedActorLit(const AActor* ActorToTest, const float IntensityThreshold = 0.5f);

	template <typename T>
	FORCEINLINE static TArray<T*> SmartPtrToRawPtr_Array(const TArray<TObjectPtr<T>>& InArray)
	{
		TArray<T*> RawArray;
		RawArray.SetNumUninitialized(InArray.Num());
		for (int32 i = 0; i < InArray.Num(); ++i)
		{
			RawArray[i] = InArray[i].Get();
		}
		return RawArray;
	}

	template <typename T>
	FORCEINLINE static TArray<TObjectPtr<T>> RawPtrToSmartPtr_Array(const TArray<T*>& InArray)
	{
		TArray<TObjectPtr<T>> ObjectPtrArray;
		ObjectPtrArray.SetNumUninitialized(InArray.Num());
		for (int32 i = 0; i < InArray.Num(); ++i)
		{
			ObjectPtrArray[i] = InArray[i];
		}
		return ObjectPtrArray;
	}

	UFUNCTION(BlueprintPure, BlueprintCallable, meta = (DisplayName = "Limb to Socket", WorldContext = "WorldContextObject"), Category= "LXR|Socket")
	static FName LimbToSocketName(const UObject* WorldContextObject, EMannequinLimb InLimb);

	static void DebugOnGameThread(TUniqueFunction<void()> Function);

	UFUNCTION(BlueprintPure, BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category= "LXR|Source")
	static FLightSourceData MakeLightSourceData(ULightComponent* InComponent, float Multiplier);

private:
	static bool CheckDirectionalLight(const ULightComponent& LightSourceComponent, const FTraceTaskData& TraceTaskData, const FVector& Start, const bool DebugDraw);
	static bool CheckDistance(const ULXRSourceComponent& LightSourceComponent, const FTraceTaskData& TraceTaskData, const ULightComponent& LightComponent, const FVector& Start, const FVector& End, const bool DebugDraw);
	static bool CheckAttenuation(float AttenuationRadiusSQR, const FTraceTaskData& TraceTaskData, const FVector& Start, const FVector& End,const  bool DebugDraw);
	static bool CheckDirection(const ULightComponent& LightComponent, const FTraceTaskData& TraceTaskData, const FVector& Start, const FVector& End,const  bool DebugDraw);
	static bool CheckIfInsideSpotOrRect(const ULightComponent& LightComponent, const FTraceTaskData& TraceTaskData, const FVector& Start, const FVector& End, bool IsSpot,const  bool DebugDraw);
};