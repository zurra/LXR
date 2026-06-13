// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.


#include "LXRFunctionLibrary.h"
#include "MathUtil.h"
#include "ILXRSilhouette.h"
#include "LXRMemoryComponent.h"
#include "LXRSilhouetteDetectionComponent.h"
#include "LXRSourceComponent.h"
#include "LXR_Shared.h"
#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetTextLibrary.h"
#include "Tasks/GenerateSilhouetteTargetsAsyncTask.h"
#include "Async/Async.h"
#include "UObject/UObjectIterator.h"


uint32 FDominantColor::GetTypeHash(const FDominantColor& Other)
{
	const uint32 Hash = FCrc::MemCrc32(&Other, sizeof(FDominantColor));
	return Hash;
}

FDominantColor ULXRFunctionLibrary::GetDominantColor(const FLinearColor& InColor)
{
	float MinValueOfColorComponents = GetMinOfColorChannels(InColor);
	float MaxValueOfColorComponents = GetMaxOfColorChannels(InColor);

	FDominantColor DominantColor;

	if (MinValueOfColorComponents == MaxValueOfColorComponents)
	{
		return DominantColor;
	}

	if (InColor.R == MaxValueOfColorComponents)
	{
		DominantColor.Color = EDominantColor::Red;
		DominantColor.ColorValue = InColor.R;
		return DominantColor;
	}

	if (InColor.G == MaxValueOfColorComponents)
	{
		DominantColor.Color = EDominantColor::Green;
		DominantColor.ColorValue = InColor.G;
		return DominantColor;
	}

	if (InColor.B == MaxValueOfColorComponents)
	{
		DominantColor.Color = EDominantColor::Blue;
		DominantColor.ColorValue = InColor.B;
		return DominantColor;
	}


	return DominantColor;
}

FDominantColor ULXRFunctionLibrary::GetSecondDominantColor(const FLinearColor& InColor)
{
	const FDominantColor DominantColor = GetDominantColor(InColor);
	FDominantColor SecondDominantColor;

	switch (DominantColor.Color)
	{
	case EDominantColor::None:
		break;
	case EDominantColor::Red:
		{
			const float MaxValueOfColorComponents = FMath::Max(InColor.G, InColor.B);

			if (InColor.G == InColor.B)
				return SecondDominantColor;

			if (InColor.G == MaxValueOfColorComponents)
			{
				SecondDominantColor.Color = EDominantColor::Green;
				SecondDominantColor.ColorValue = InColor.G;
			}
			else
			{
				SecondDominantColor.Color = EDominantColor::Blue;
				SecondDominantColor.ColorValue = InColor.B;
			}
		}
		break;
	case EDominantColor::Green:
		{
			const float MaxValueOfColorComponents = FMath::Max(InColor.R, InColor.B);

			if (InColor.R == InColor.B)
				return SecondDominantColor;

			if (InColor.R == MaxValueOfColorComponents)
			{
				SecondDominantColor.Color = EDominantColor::Red;
				SecondDominantColor.ColorValue = InColor.R;
			}
			else
			{
				SecondDominantColor.Color = EDominantColor::Blue;
				SecondDominantColor.ColorValue = InColor.B;
			}
		}
		break;
	case EDominantColor::Blue:
		{
			const float MaxValueOfColorComponents = FMath::Max(InColor.R, InColor.G);

			if (InColor.R == InColor.G)
				return SecondDominantColor;

			if (InColor.R == MaxValueOfColorComponents)
			{
				SecondDominantColor.Color = EDominantColor::Red;
				SecondDominantColor.ColorValue = InColor.R;
			}
			else
			{
				SecondDominantColor.Color = EDominantColor::Green;
				SecondDominantColor.ColorValue = InColor.G;
			}
		}
		break;
	default: ;
	}
	return SecondDominantColor;
}

FLinearColor ULXRFunctionLibrary::GetInverseChannels(const FLinearColor& InColor)
{
	FVector ColorVector = FVector(InColor);
	ColorVector = ClampVector(ColorVector, FVector::ZeroVector, FVector::One());
	ColorVector = (ColorVector - 1).GetAbs();
	return FLinearColor(ColorVector);
}

FLinearColor ULXRFunctionLibrary::ClampTo01Range(const FLinearColor& InColor)
{
	return InColor.GetClamped(0, 1);
}

FLinearColor ULXRFunctionLibrary::DominantToLinearColor(const FDominantColor& InDominantColor)
{
	FLinearColor Temp;
	switch (InDominantColor.Color)
	{
	case EDominantColor::None:
		break;
	case EDominantColor::Red:
		Temp = FLinearColor(InDominantColor.ColorValue, 0, 0, 1);
		break;
	case EDominantColor::Green:
		Temp = FLinearColor(0, InDominantColor.ColorValue, 0, 1);
		break;
	case EDominantColor::Blue:
		Temp = FLinearColor(0, 0, InDominantColor.ColorValue, 1);
		break;
	default: ;
	}
	return Temp;
}

bool ULXRFunctionLibrary::Equal_FDominantColor(const FDominantColor& FirstColor, const FDominantColor& SecondColor)
{
	return FirstColor == SecondColor;
}

bool ULXRFunctionLibrary::ColorEqual_DominantColor(const FLinearColor& Color, const FDominantColor& DominantColor)
{
	const FDominantColor InColorDominantColor = GetDominantColor(Color);
	return InColorDominantColor == DominantColor;
}

bool ULXRFunctionLibrary::ColorApproximatelyEqualColor(const FLinearColor& InColorOne, const FLinearColor& InColorTwo)
{
	if (InColorOne.CopyWithNewOpacity(0) == InColorTwo.CopyWithNewOpacity(0))
		return true;

	const float MinValueOfColorOneComponents = GetMinOfColorChannels(InColorOne);
	const float MaxValueOfColorOneComponents = GetMaxOfColorChannels(InColorOne);

	const float MinValueOfColorTwoComponents = GetMinOfColorChannels(InColorTwo);
	const float MaxValueOfColorTwoComponents = GetMaxOfColorChannels(InColorTwo);

	// GEngine->AddOnScreenDebugMessage(-1, 0.2, FColor::Green, FString::Printf(TEXT("One Mi: %f  Ma: %f"), MinValueOfColorOneComponents, MaxValueOfColorOneComponents));
	// GEngine->AddOnScreenDebugMessage(-1, 0.2f, FColor::Red, FString::Printf(TEXT("Two Mi: %f  Ma: %f"), MinValueOfColorTwoComponents, MaxValueOfColorTwoComponents));
	if (MinValueOfColorOneComponents == MaxValueOfColorOneComponents || MinValueOfColorTwoComponents == MaxValueOfColorTwoComponents)
	{
		return false;
	}

	const FDominantColor InColorOneDominantColor = GetDominantColor(InColorOne);
	const FDominantColor InColorTwoDominantColor = GetDominantColor(InColorTwo);

	const FDominantColor InColorTwoSecondDominantColor = GetSecondDominantColor(InColorTwo);
	const FDominantColor InColorOneSecondDominantColor = GetSecondDominantColor(InColorOne);

	if (InColorOneDominantColor == InColorTwoDominantColor)
	{
		if (Round(InColorOne) == Round(InColorTwo))
			return true;
	}


	if (InColorOneDominantColor.Color == EDominantColor::None &&
		InColorOneSecondDominantColor.Color == EDominantColor::None &&
		InColorTwoDominantColor.Color == EDominantColor::None &&
		InColorTwoSecondDominantColor.Color == EDominantColor::None)
	{
		return false;
	}

	const bool FirstDominantColorSame = InColorOneDominantColor.Color == InColorTwoSecondDominantColor.Color || InColorOneDominantColor.Color == InColorTwoDominantColor.Color;
	const bool SecondDominantColorSame = InColorOneSecondDominantColor.Color == InColorTwoSecondDominantColor.Color || InColorOneSecondDominantColor.Color == InColorTwoDominantColor.Color;

	return FirstDominantColorSame && SecondDominantColorSame;
}

bool ULXRFunctionLibrary::ColorRemappedRoundedHalfEqualColor(const FLinearColor& InColorOne, const FLinearColor& InColorTwo)
{
	FLinearColor ColorOne = RemapColorRangeTo01(InColorOne);
	ColorOne = RoundToNearestHalf(ColorOne);
	FLinearColor ColorTwo = RoundToNearestHalf(InColorTwo);

	ColorOne.A = 1;
	ColorTwo.A = 1;
	return ColorOne == ColorTwo;
}

FLinearColor ULXRFunctionLibrary::ToggleColorChannels(const FLinearColor& InColor, const FLinearColor& ToggleChannels)
{
	const float CeilInR = FMath::CeilToFloat(InColor.R);
	const float CeilInG = FMath::CeilToFloat(InColor.G);
	const float CeilInB = FMath::CeilToFloat(InColor.B);

	const bool ZeroR = ToggleChannels.R > 0 && InColor.R > 0 ? true : false;
	const bool ZeroG = ToggleChannels.G > 0 && InColor.G > 0 ? true : false;
	const bool ZeroB = ToggleChannels.B > 0 && InColor.B > 0 ? true : false;

	const float R = ZeroR ? 0 : ToggleChannels.R > 0 ? FMath::Abs(CeilInR - ToggleChannels.R) : InColor.R;
	const float G = ZeroG ? 0 : ToggleChannels.G > 0 ? FMath::Abs(CeilInG - ToggleChannels.G) : InColor.G;
	const float B = ZeroB ? 0 : ToggleChannels.B > 0 ? FMath::Abs(CeilInB - ToggleChannels.B) : InColor.B;

	return FLinearColor(R, G, B, 1);
}

FLinearColor ULXRFunctionLibrary::RemapColorRangeTo01(const FLinearColor& InColor)
{
	const float MaxValueOfColorComponents = GetMaxOfColorChannels(InColor);

	const float R = InColor.R != 0 ? InColor.R / MaxValueOfColorComponents : 0;
	const float G = InColor.G != 0 ? InColor.G / MaxValueOfColorComponents : 0;
	const float B = InColor.B != 0 ? InColor.B / MaxValueOfColorComponents : 0;

	return FLinearColor(R, G, B, 1);
}

float ULXRFunctionLibrary::GetMaxOfColorChannels(const FLinearColor& InColor)
{
	return FMath::Max(FMath::Max(InColor.R, InColor.G), InColor.B);
}

float ULXRFunctionLibrary::GetMinOfColorChannels(const FLinearColor& InColor)
{
	return FMath::Min(FMath::Min(InColor.R, InColor.G), InColor.B);
}

FLinearColor ULXRFunctionLibrary::RoundToNearestHalf(const FLinearColor& InColor)
{
	const float R = InColor.R != 0 ? FMath::RoundToFloat(InColor.R * 2) / 2 : 0;
	const float G = InColor.G != 0 ? FMath::RoundToFloat(InColor.G * 2) / 2 : 0;
	const float B = InColor.B != 0 ? FMath::RoundToFloat(InColor.B * 2) / 2 : 0;
	return FLinearColor(R, G, B, 1);
}

FLinearColor ULXRFunctionLibrary::Round(const FLinearColor& InColor)
{
	const float R = FMath::RoundToFloat(InColor.R);
	const float G = FMath::RoundToFloat(InColor.G);
	const float B = FMath::RoundToFloat(InColor.B);
	return FLinearColor(R, G, B, 1);
}

FLinearColor ULXRFunctionLibrary::GetLinearColorArrayAverage(const TArray<FLinearColor>& InColors)
{
	FLinearColor Sum = FLinearColor::Black;
	FLinearColor Average = FLinearColor::Black;

	if (InColors.Num() > 0)
	{
		for (int32 VecIdx = 0; VecIdx < InColors.Num(); VecIdx++)
		{
			Sum += InColors[VecIdx];
		}

		Average = Sum / static_cast<float>(InColors.Num());
	}

	return Average;
}

FName ULXRFunctionLibrary::LimbToSocketName(const UObject* WorldContextObject, EMannequinLimb InLimb)
{
	switch (InLimb)
	{
	case EMannequinLimb::Head:
		return FName("head");
	case EMannequinLimb::Hand_R:
		return FName("hand_r");
	case EMannequinLimb::Hand_L:
		return FName("hand_l");
	case EMannequinLimb::LowerArm_R:
		return FName("lowerarm_r");
	case EMannequinLimb::LowerArm_L:
		return FName("lowerarm_l");
	case EMannequinLimb::UpperArm_R:
		return FName("upperarm_r");
	case EMannequinLimb::UpperArm_L:
		return FName("upperarm_l");
	case EMannequinLimb::Thigh_R:
		return FName("thigh_r");
	case EMannequinLimb::Thigh_L:
		return FName("thigh_l");
	case EMannequinLimb::Calf_R:
		return FName("calf_r");
	case EMannequinLimb::Calf_L:
		return FName("calf_l");
	case EMannequinLimb::Foot_R:
		return FName("foot_r");
	case EMannequinLimb::Foot_L:
		return FName("foot_l");
	case EMannequinLimb::Chest:
		return FName("spine_02");
	default: ;
	}
	return FName();
}

void ULXRFunctionLibrary::DebugOnGameThread(TUniqueFunction<void()> Function)
{
	AsyncTask(ENamedThreads::GameThread, [CapturedFunction = MoveTemp(Function)]() mutable
	{
		CapturedFunction();
	});
}

FLightSourceData ULXRFunctionLibrary::MakeLightSourceData(ULightComponent* InComponent, float Multiplier)
{
	FLightSourceData Data = FLightSourceData();
	Data.LightComponent.OverrideComponent = InComponent;
	Data.LightData = Multiplier;
	return Data;
}

bool ULXRFunctionLibrary::CheckDirectionalLight(const ULightComponent& LightComponent, const FTraceTaskData& TraceTaskData, const FVector& End, const bool DebugDraw)
{
	constexpr double DirectionalLightTraceDistance = 50000;
	const TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
	FCollisionQueryParams Params("CheckDirectionalLight", SCENE_QUERY_STAT_ONLY(KismetTraceUtils), false);

	const FVector DirectionalForwardInverse = LightComponent.GetForwardVector() * -1;
	const FVector Start = End + DirectionalForwardInverse.GetSafeNormal() * DirectionalLightTraceDistance;

	TArray<AActor*> ActorsToIgnore;

	ActorsToIgnore.Append(Cast<ULXRSourceComponent>(LightComponent.GetOwner()->GetComponentByClass(ULXRSourceComponent::StaticClass()))->GetMyOverlappingActors());
	ActorsToIgnore.Append(Cast<ULXRSourceComponent>(LightComponent.GetOwner()->GetComponentByClass(ULXRSourceComponent::StaticClass()))->GetIgnoreVisibilityActors());
	ActorsToIgnore.AddUnique(LightComponent.GetOwner());
	ActorsToIgnore.Add(TraceTaskData.Instigator.Get());

	Params.AddIgnoredActors(ActorsToIgnore);

	bool passed = false;
	if (!LightComponent.GetWorld()->LineTraceTestByChannel(Start, End, TraceChannel, Params))
	{
		passed = true;
	}
#if UE_ENABLE_DEBUG_DRAWING

	if (DebugDraw)
	{
		ULXRFunctionLibrary::DebugOnGameThread([WeakLightComponent = MakeWeakObjectPtr(&LightComponent), Start, End, passed, TraceChannel,Params]
		{
			if (!WeakLightComponent.IsValid()) return;
			FHitResult HitResult;

			WeakLightComponent.Get()->GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, TraceChannel, Params);

			DrawDebugLine(WeakLightComponent->GetWorld(), Start, passed ? End : HitResult.Location, passed ? FColor::Green : FColor::Red, false, 0.1f, 0, 2);

			if (!passed)
			{
				auto TimeSine = (FMath::Sin(WeakLightComponent->GetWorld()->GetTimeSeconds() * 1.2f) + 2);
				DrawDebugPoint(WeakLightComponent->GetWorld(), HitResult.Location, 10.f * TimeSine, FColor::Magenta, false, 0.1f);
				if (const auto HitComponent = HitResult.GetComponent())
				{
					UE_LOG(LogLXR, Log, TEXT("Relevancy::CheckDirectionalLight %s:  Hit Actor and component: %s : %s  "), *WeakLightComponent->GetOwner()->GetName(), *HitResult.GetActor()->GetName(), *HitComponent->GetName());
				}
				else
				{
					UE_LOG(LogLXR, Log, TEXT("Relevancy::CheckDirectionalLight %s:  Hit Actor %s  "), *WeakLightComponent->GetOwner()->GetName(), *HitResult.GetActor()->GetName());
				}
			}
		});
	}

#endif
	return passed;
}

bool ULXRFunctionLibrary::CheckDistance(const ULXRSourceComponent& LightSourceComponent, const FTraceTaskData& TraceTaskData, const ULightComponent& LightComponent, const FVector& Start, const FVector& End,
                                        const bool DebugDraw)
{
	const bool bIsSpotLight = LightComponent.IsA(USpotLightComponent::StaticClass());
	const bool bIsPointLight = LightComponent.IsA(UPointLightComponent::StaticClass()) && !bIsSpotLight;
	const float Attenuation = bIsSpotLight
		                          ? Cast<USpotLightComponent>(&LightComponent)->AttenuationRadius
		                          : bIsPointLight
		                          ? Cast<UPointLightComponent>(&LightComponent)->AttenuationRadius
		                          : Cast<URectLightComponent>(&LightComponent)->AttenuationRadius;

	const float DistanceToCheck = Attenuation * LightSourceComponent.AttenuationMultiplierToBeRelevant;
	const float DistanceSqr = FVector::DistSquared(Start, End);

	if (DistanceSqr < DistanceToCheck * DistanceToCheck)
		return true;
	return false;
}

bool ULXRFunctionLibrary::CheckDirection(const ULightComponent& LightComponent, const FTraceTaskData& TraceTaskData, const FVector& Start, const FVector& End, const bool DebugDraw)
{
	const float Dot = FVector::DotProduct(LightComponent.GetForwardVector(), (End - Start).GetSafeNormal());
	if (Dot > 0)
		return true;
	return false;
}

bool ULXRFunctionLibrary::CheckAttenuation(float AttenuationRadiusSQR, const FTraceTaskData& TraceTaskData, const FVector& Start, const FVector& End, const bool DebugDraw)
{
	const float DistanceSqr = FVector::DistSquared(Start, End);
	return DistanceSqr < AttenuationRadiusSQR;
}

bool ULXRFunctionLibrary::CheckIfInsideSpotOrRect(const ULightComponent& LightComponent, const FTraceTaskData& TraceTaskData, const FVector& End, const FVector& Start, bool IsSpot, const bool DebugDraw)
{
	auto CheckForLinePlaneIntersectionAndFindClosestPointOnSegment(
		[&](const FVector& LineStart, const FVector& LineEnd, const FVector& PlaneNormal, const FVector& EdgeStart, const FVector& EdgeEnd, FVector& Intersection)
		{
			const FVector RayDir = LineEnd - LineStart;
			const float DotProduct = RayDir | PlaneNormal;

			// Check ray is not parallel to plane
			if (FMath::IsNearlyZero(DotProduct))
			{
				Intersection = FVector::ZeroVector;
				return false;
			}

			const float T = (((EdgeEnd - LineStart) | PlaneNormal) / DotProduct);

			// Check intersection is not outside line segment
			if (T < 0.0f || T > 1.0f)
			{
				Intersection = FVector::ZeroVector;
				return false;
			}

			// Calculate intersection point
			Intersection = LineStart + RayDir * static_cast<double>(T);

			const FVector ClosestPoint = FMath::ClosestPointOnSegment(Intersection, EdgeStart, EdgeEnd);
			if (FVector::DistSquared(ClosestPoint, Intersection) < FMath::Square(0.01f))
				return true;

			return false;
		});

	auto CheckAngle = [&](const FVector& ReferenceDirection, const FVector& InStart, const FVector& InEnd, float TargetAngleRadians)
	{
		const FVector DirectionToTarget = (InEnd - InStart).GetSafeNormal();
		return FVector::DotProduct(ReferenceDirection, DirectionToTarget) > FMath::Cos(TargetAngleRadians);
	};


	if (IsSpot)
	{
		const FVector& Forward = LightComponent.GetForwardVector();
		const float OuterConeAngleRadians = FMath::DegreesToRadians(Cast<USpotLightComponent>(&LightComponent)->OuterConeAngle);
		return CheckAngle(Forward, End, Start, OuterConeAngleRadians);
	}

	{
		const FVector DetectionProjectedToRectPlane = FVector::PointPlaneProject(Start, End, LightComponent.GetForwardVector());
		FVector Test = LightComponent.GetComponentTransform().TransformPosition(LightComponent.GetComponentTransform().InverseTransformPosition(DetectionProjectedToRectPlane) * -1);

		const URectLightComponent* RectLight = Cast<URectLightComponent>(&LightComponent);
		const float SourceHeight = RectLight->SourceHeight;
		const float SourceWidth = RectLight->SourceWidth;
		const float BarnDoorAngle = RectLight->BarnDoorAngle;
		const float BarnDoorLength = RectLight->BarnDoorLength;
		const float BarnMaxAngle = GetRectLightBarnDoorMaxAngle();
		const float AngleRad = FMath::DegreesToRadians(FMath::Clamp(BarnDoorAngle, 0.f, BarnMaxAngle));
		const float BarnDepth = FMath::Cos(AngleRad) * BarnDoorLength;
		const float BarnExtent = FMath::Sin(AngleRad) * BarnDoorLength;
		const FVector AbsDetectionProjectedToRectPlaneInverse = LightComponent.GetComponentToWorld().InverseTransformPosition(DetectionProjectedToRectPlane).GetAbs();

		const FVector V1 = LightComponent.GetComponentTransform().TransformPosition(FVector(0.0f, +0.5f * SourceWidth, +0.5f * SourceHeight));
		const FVector V2 = LightComponent.GetComponentTransform().TransformPosition(FVector(0.0f, +0.5f * SourceWidth, -0.5f * SourceHeight));
		const FVector V3 = LightComponent.GetComponentTransform().TransformPosition(FVector(0.0f, -0.5f * SourceWidth, +0.5f * SourceHeight));
		const FVector V4 = LightComponent.GetComponentTransform().TransformPosition(FVector(0.0f, -0.5f * SourceWidth, -0.5f * SourceHeight));
		const FVector BarnV1 = LightComponent.GetComponentTransform().TransformPosition(FVector(BarnDepth, +0.5f * SourceWidth + BarnExtent, +0.5f * SourceHeight + BarnExtent));
		const FVector BarnV2 = LightComponent.GetComponentTransform().TransformPosition(FVector(BarnDepth, +0.5f * SourceWidth + BarnExtent, -0.5f * SourceHeight - BarnExtent));
		const FVector BarnV3 = LightComponent.GetComponentTransform().TransformPosition(FVector(BarnDepth, -0.5f * SourceWidth - BarnExtent, +0.5f * SourceHeight + BarnExtent));
		const FVector BarnV4 = LightComponent.GetComponentTransform().TransformPosition(FVector(BarnDepth, -0.5f * SourceWidth - BarnExtent, -0.5f * SourceHeight - BarnExtent));

		// UE_LOG(LogLXR, Warning, TEXT("DetectionPlane:%s, H:%f , W:%f" ), *AbsDetectionProjectedToRectPlaneInverse.ToString(), SourceHeight/2, SourceWidth/2);

		if (AbsDetectionProjectedToRectPlaneInverse.Z < SourceHeight / 2 && AbsDetectionProjectedToRectPlaneInverse.Y < SourceWidth / 2)
			return true;

		//EDGE1 V1 V3
		//EDGE2 V2 V4
		//EDGE3 V1 V2
		//EDGE4 V3 V4

		FVector EdgeIntersection;

		if (CheckForLinePlaneIntersectionAndFindClosestPointOnSegment(End, Test, LightComponent.GetUpVector(), V1, V3, EdgeIntersection))
		{
			const FVector BarnMid = ((BarnV2 - BarnV4) * 0.5) + BarnV4;
			const float Dot = FVector::DotProduct(LightComponent.GetForwardVector(), (BarnMid - EdgeIntersection).GetSafeNormal());
			const float AngleInRadians = acosf(Dot);

			const bool AnglePassed = CheckAngle(LightComponent.GetForwardVector(), EdgeIntersection, Start, AngleInRadians);


			return AnglePassed;
		}

		if (CheckForLinePlaneIntersectionAndFindClosestPointOnSegment(End, Test, LightComponent.GetUpVector() * -1, V2, V4, EdgeIntersection))
		{
			const FVector BarnMid = ((BarnV1 - BarnV3) * 0.5) + BarnV3;
			const float Dot = FVector::DotProduct(LightComponent.GetForwardVector(), (BarnMid - EdgeIntersection).GetSafeNormal());
			const float AngleInRadians = acosf(Dot);
			const bool AnglePassed = CheckAngle(LightComponent.GetForwardVector(), EdgeIntersection, Start, AngleInRadians);


			return AnglePassed;
		}

		if (CheckForLinePlaneIntersectionAndFindClosestPointOnSegment(End, Test, LightComponent.GetRightVector(), V1, V2, EdgeIntersection))
		{
			const FVector BarnMid = ((BarnV3 - BarnV4) * 0.5) + BarnV4;
			const float Dot = FVector::DotProduct(LightComponent.GetForwardVector(), (BarnMid - EdgeIntersection).GetSafeNormal());
			const float AngleInRadians = acosf(Dot);

			const bool AnglePassed = CheckAngle(LightComponent.GetForwardVector(), EdgeIntersection, Start, AngleInRadians);


			return AnglePassed;
		}
		if (CheckForLinePlaneIntersectionAndFindClosestPointOnSegment(End, Test, LightComponent.GetRightVector() * -1, V3, V4, EdgeIntersection))
		{
			const FVector BarnMid = ((BarnV1 - BarnV2) * 0.5) + BarnV2;
			const float Dot = FVector::DotProduct(LightComponent.GetForwardVector(), (BarnMid - EdgeIntersection).GetSafeNormal());
			const float AngleInRadians = acosf(Dot);

			const bool AnglePassed = CheckAngle(LightComponent.GetForwardVector(), EdgeIntersection, Start, AngleInRadians);

			return AnglePassed;
		}
	}

	return false;
}


void ULXRFunctionLibrary::GetRelevantLights(const UObject* WorldContextObject, const TArray<FVector>& TraceTargets, TSet<TWeakObjectPtr<ULXRSourceComponent>>& RelevantLights)
{
	const ULXRSubsystem* LXRSubsystem = WorldContextObject->GetWorld()->GetSubsystem<ULXRSubsystem>();
	if (LXRSubsystem->GetOctree().IsValid())
	{
		FBox Bounds(ForceInit);

		for (const FVector& Vector : TraceTargets)
		{
			// Extend the bounds to include the vector
			Bounds += Vector;
		}

		FVector Center, Extents;
		Bounds.GetCenterAndExtents(Center, Extents);
		if (Extents.GetMax() < 1000)
		{
			Extents = FVector(1000);
		}
		FBoxCenterAndExtent OctreeBounds;
		OctreeBounds.Center = Center;
		OctreeBounds.Extent = Extents;

		LXRSubsystem->GetOctreeLights(OctreeBounds, RelevantLights);
	}
	else
	{
		if (RelevantLights.Num() == 0)
		{
			TArray<TWeakObjectPtr<ULXRSourceComponent>> AllLights = LXRSubsystem->GetAllLights();
			if (AllLights.Num() > 0)
			{
				for (auto& Light : AllLights)
				{
					RelevantLights.FindOrAdd(Light);
				}
			}
		}
	}
}

TSet<FVector> ULXRFunctionLibrary::GenerateGridTargets(const UObject* WorldContextObject, FVector Center, float Radius, float Density)
{
	TSet<FVector> GridPoints;
	const int32 ItemCount = FPlatformMath::TruncToInt((Radius * 2.0f / Density) + 1);
	const int32 ItemCountHalf = ItemCount / 2;
	for (int32 IndexX = 0; IndexX < ItemCount; ++IndexX)
	{
		for (int32 IndexY = 0; IndexY < ItemCount; ++IndexY)
		{
			const FVector GridPoint = Center - FVector(Density * (IndexX - ItemCountHalf), Density * (IndexY - ItemCountHalf), 0);
			GridPoints.Add(GridPoint);
		}
	}
	return GridPoints;
}

TSet<FVector> ULXRFunctionLibrary::GenerateCylinderTargets(const UObject* WorldContextObject, FVector Center, FVector Direction, int CylinderSegments, int CylinderRadius, int CylinderDistance, int DistancePerSegment)
{
	auto Round = [](const FVector& V) -> FVector
	{
		return FVector(
			FMathd::Round(V[0]),
			FMathd::Round(V[1]),
			FMathd::Round(V[2]));
	};
	TSet<FVector> Points;
	TArray<FVector> CylinderEdges;
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	FVector End;
	FVector& Start = Center;
	const bool DrawDebug = false;

	// Calculate the rotation matrix based on the direction
	UE::Math::TMatrix<double> RotationMatrix = FRotationMatrix::MakeFromX(FVector::RightVector);

	// Calculate the axis vector of the cylinder
	FVector AxisVector = FVector::ForwardVector;
	for (int32 SideIndex = 0; SideIndex < CylinderSegments; ++SideIndex)
	{
		// Calculate the angle for the current side
		float Theta = 360.0f * (static_cast<float>(SideIndex) / static_cast<float>(CylinderSegments));

		// Convert angle to radians
		double ThetaRad = FMath::DegreesToRadians(Theta + World->RealTimeSeconds * 150);

		// Calculate the position on the cylinder
		FVector PointOnCylinder = Start + RotationMatrix.TransformVector(AxisVector * CylinderRadius * FMath::Cos(ThetaRad))
			+ RotationMatrix.GetUnitAxis(EAxis::Z) * CylinderRadius * FMath::Sin(ThetaRad);

		// Do something with the PointOnCylinder, such as spawning a mesh or drawing a debug point
		// ...
		CylinderEdges.AddUnique(PointOnCylinder);
		// if (bDrawDebug)
	}

	// for (int c = 0; c < CylinderEdges.Num(); ++c)
	// {
	// 	float Wrapped = FMath::Wrap(c + 1, -1, CylinderEdges.Num() - 1);
	// 	FVector C1 = UKismetMathLibrary::RotateAngleAxis(Rotator.UnrotateVector(Start-CylinderEdges[c]),90,FVector::UpVector)+Start;
	// 	FVector C2 = UKismetMathLibrary::RotateAngleAxis(Rotator.UnrotateVector(Start-CylinderEdges[Wrapped]),90,FVector::UpVector)+Start;
	//
	// 	DrawDebugLine(GetWorld(), C1, C2, FColor::Emerald, false, GetComponentTickInterval() * 2, 0, 5);
	// }

	{
		for (FVector& CylinderStart : CylinderEdges)
		{
			End = CylinderStart + Direction * CylinderDistance;
			float Distance = FVector::Dist(Start, End);
			DistancePerSegment = 100;
			int Segments = FMath::RoundToInt(Distance / DistancePerSegment);

			// DrawDebugLine(World, Start, End, FColor::Emerald, false, 5, 0, 1);

			for (int i = 1; i < Segments; ++i)
			{
				// if (FMath::RandBool())
				//  	continue;
				FVector& StartToUse = CylinderStart;

				for (int Count = 0; Count <= 1; ++Count)
				{
					if (Count == 1)
					{
						// StartToUse = CylinderStart + (Start - CylinderStart) * 0.5;
					}

					auto DirectionToCenter = (StartToUse - Start).GetSafeNormal();
					int IteratorDistance = i * DistancePerSegment;
					float SinMod = FMath::Sin(static_cast<float>(IteratorDistance * 5));
					// float CosMod = (FMath::Cos(GetWorld()->GetRealTimeSeconds()*5)+1)*0.5f;
					float DistanceToAdd = FMath::Wrap<float>(IteratorDistance + FMath::FRand() * CylinderDistance, 100.f, CylinderDistance);

					FVector TracePoint = FVector::ForwardVector * static_cast<double>(DistanceToAdd);
					// auto CurrentDistance = TracePoint.Size();
					auto CurrentDistancePercentFromSenseDistance = DistanceToAdd / CylinderDistance;

					bool UseTracePoint = UKismetMathLibrary::RandomBoolWithWeight(FMath::Max(0.1f, CurrentDistancePercentFromSenseDistance * 1));
					if (UseTracePoint)
					{
						// TracePoint = TracePoint + DirectionToCenter * (SinMod * (CylinderRadius * UKismetMathLibrary::Lerp(1, 4, CurrentDistancePercentFromSenseDistance)));
						TracePoint = TracePoint + DirectionToCenter * static_cast<double>((SinMod * CylinderRadius));
						FRotator Rotator = UKismetMathLibrary::MakeRotFromX(Direction);
						TracePoint = Rotator.RotateVector(TracePoint) + StartToUse;
						// FVector TracePoint2 = TracePoint+RotatedDirection*-CylinderRadius;
						// TracePoint = TracePoint+(Start-End).GetSafeNormal()*CosMod*100;

						// DrawDebugPoint(GetWorld(), TracePoint2, 5.f, FColor::Yellow, false, GetComponentTickInterval());
						Points.Add(Round(TracePoint));
					}
				}
			}
			// DrawDebugLine(GetWorld(), ZeroAngleEnd, End, FColor::Emerald, false, DrawTime, 0, 1);
		}
	}
	return Points;
}

TSet<FVector> ULXRFunctionLibrary::GenerateConeTraceTargets(const UObject* WorldContextObject, const FVector& Location, const FVector& Direction, int ConeAngle, int ConeSides, int ConeRadius, int ConeDistance,
                                                            int DistancePerSegment)
{
	auto Round = [](const FVector& V) -> FVector
	{
		return FVector(
			FMathd::Round(V[0]),
			FMathd::Round(V[1]),
			FMathd::Round(V[2]));
	};
	TSet<FVector> Points;
	TArray<FVector> ConeEdges;
	FVector Start = Location;
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	FVector RotatedDirection;
	RotatedDirection = UKismetMathLibrary::RotateAngleAxis(FVector::ForwardVector, ConeAngle, FVector::LeftVector);

	FVector A = RotatedDirection * ConeDistance;
	FVector B = FVector::ForwardVector * ConeDistance;
	FVector P = UKismetMathLibrary::ProjectVectorOnToVector(A, B);

	bool bDrawDebug = false;

	double CapsuleRadius = (P - A).Length();
	// Calculate the rotation matrix based on the direction
	UE::Math::TMatrix<double> RotationMatrix = FRotationMatrix::MakeFromX(FVector::RightVector);

	// Calculate the axis vector of the Cone
	FVector AxisVector = FVector::ForwardVector;
	for (int32 SideIndex = 0; SideIndex < ConeSides; ++SideIndex)
	{
		// Calculate the angle for the current side
		float Theta = 360.0f * (static_cast<float>(SideIndex) / static_cast<float>(ConeSides));

		// Convert angle to radians
		double ThetaRad = FMath::DegreesToRadians(Theta + World->RealTimeSeconds * 150);
		// float ThetaRad = FMath::DegreesToRadians(Theta);

		// Calculate the position on the Cone
		FVector PointOnCone = B + RotationMatrix.TransformVector(AxisVector * CapsuleRadius * FMath::Cos(ThetaRad))
			+ RotationMatrix.GetUnitAxis(EAxis::Z) * CapsuleRadius * FMath::Sin(ThetaRad);

		ConeEdges.AddUnique(PointOnCone);
	}

	for (FVector& EdgeEnd : ConeEdges)
	{
		int Segments = FMath::RoundToInt(static_cast<float>(ConeDistance) / DistancePerSegment);

		if (bDrawDebug)
		{
			DrawDebugLine(World, Start, Start + RotatedDirection * ConeDistance, FColor::Emerald, false, 5.f, 0, 1);
		}

		for (int i = 1; i < Segments; ++i)
		{
			// if (FMath::RandBool())
			// 	continue;
			for (int Count = 0; Count <= 1; ++Count)
			{
				int IteratorDistance = i * DistancePerSegment;
				FVector ConeEdgeEnd = EdgeEnd.GetSafeNormal() * IteratorDistance;
				FVector Center = UKismetMathLibrary::ProjectVectorOnToVector(ConeEdgeEnd, B);
				FVector DirectionToUse = EdgeEnd.GetSafeNormal();
				if (Count == 1)
				{
					DirectionToUse = FMath::LerpStable(FVector::ForwardVector, EdgeEnd.GetSafeNormal(), 0.5);
				}

				FVector DirectionToCenter = (Center - ConeEdgeEnd);
				double DistanceToCenter = DirectionToCenter.Length();
				DirectionToCenter = DirectionToCenter.GetSafeNormal();

				float SinMod = (FMath::Sin(static_cast<float>(IteratorDistance * 5 + World->GetRealTimeSeconds() * 2)) + 1) * 0.5;
				// float CosMod = (FMath::Cos(GetWorld()->GetRealTimeSeconds()*5)+1)*0.5f;
				auto DistanceToAdd = FMath::Wrap<float>(IteratorDistance + World->GetRealTimeSeconds() * 150, 100.f, ConeDistance);

				FVector TracePoint = DirectionToUse.GetSafeNormal() * static_cast<double>(DistanceToAdd);
				FVector TracePoint2 = EdgeEnd.GetSafeNormal() * static_cast<double>(DistanceToAdd);
				auto CurrentDistance = TracePoint.Size();
				auto CurrentDistancePercentFromSenseDistance = DistanceToAdd / ConeDistance;

				// bool UseTracePoint = UKismetMathLibrary::RandomBoolWithWeight(FMath::Max(0.1f, CurrentDistancePercentFromSenseDistance * 1));
				TracePoint = TracePoint + DirectionToCenter * (SinMod * (DistanceToCenter * CurrentDistancePercentFromSenseDistance));
				// FVector TracePoint2 = TracePoint+RotatedDirection*-CapsuleRadius;
				// TracePoint = TracePoint+(Start-End).GetSafeNormal()*CosMod*100;
				// TracePoint = TracePoint+Start;
				// DrawDebugPoint(GetWorld(), TracePoint, 10.f, FColor::Green, false, GetComponentTickInterval());
				FRotator Rotator = UKismetMathLibrary::MakeRotFromX(Direction);
				TracePoint = Rotator.RotateVector(TracePoint) + Start;
				TracePoint2 = Rotator.RotateVector(TracePoint2) + Start;

				if (bDrawDebug)
				{
					DrawDebugPoint(World, DirectionToCenter, 10.f, FColor::Green, false, 5);
				}

				bool AddNewTracePoint = true;

				for (auto& Point : Points)
				{
					if ((Point - TracePoint).Size() < 50)
					{
						AddNewTracePoint = false;
						break;
					}
				}

				if (AddNewTracePoint)
					Points.Add(Round(TracePoint));

				Points.Add(Round(TracePoint2));
			}
		}
	}
	return Points;
}

TSet<FVector> ULXRFunctionLibrary::GenerateSilhouetteTraceTargets(const UObject* WorldContextObject, const AActor* FromActor, const AActor* ToActor, FSilhouetteTaskData& SilhouetteTaskDat)
{
	TSet<FVector> Points;
	FSilhouetteTaskData TaskData;
	FSilhouetteTraceTaskParams SilhouetteTraceTaskParams = FSilhouetteTraceTaskParams(ECheckQuality::Medium);
	TaskData.Instigator = const_cast<AActor*>(FromActor);
	TaskData.DetectionComponent = ToActor->FindComponentByClass<ULXRDetectionComponent>();
	FetchSilhouetteTraceParams(*TaskData.Instigator, *TaskData.DetectionComponent->GetOwner(), TaskData);
	(new FAutoDeleteAsyncTask<FGenerateSilhouetteTargetsAsyncTask>(TaskData, SilhouetteTraceTaskParams))->StartSynchronousTask();
	for (auto& TracePoint : TaskData.TracePoints)
	{
		Points.Add(TracePoint);
	}
	return Points;
}

TSet<FVector> ULXRFunctionLibrary::GenerateCircleTraceTargets(const UObject* WorldContextObject, FVector const& Center, FVector const& Forward, float Radius, int32 Segments)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR("GenerateCircleTraceTargets");

	TSet<FVector> Points;
	const int& Edges = Segments;
	bool bDrawDebug = false;
	const FVector& Start = Center;
	// Calculate the rotation matrix based on the direction
	const UE::Math::TMatrix<double> RotationMatrix = FRotationMatrix::MakeFromX(Forward.ToOrientationQuat().RotateVector(FVector::RightVector));


	// Calculate the axis vector of the cylinder
	const FVector& AxisVector = FVector::ForwardVector;

	for (int32 SideIndex = 0; SideIndex < Edges; ++SideIndex)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("GenerateSilhouetteTargets: Cylinder");
		// Calculate the angle for the current side
		double Theta = 360.0f * (static_cast<float>(SideIndex) / static_cast<float>(Edges));

		// Convert angle to radians
		double ThetaRad;
		if (IsValid(WorldContextObject))
		{
			ThetaRad = FMath::DegreesToRadians(Theta + WorldContextObject->GetWorld()->RealTimeSeconds * 150);
		}
		else
		{
			ThetaRad = FMath::DegreesToRadians(Theta);
		}

		// Calculate the position on the cylinder
		FVector PointOnCylinder = Start + RotationMatrix.TransformVector(AxisVector * Radius * FMath::Cos(ThetaRad))
			+ RotationMatrix.GetUnitAxis(EAxis::Z) * Radius * FMath::Sin(ThetaRad);

		// Do something with the PointOnCylinder, such as spawning a mesh or drawing a debug point
		// ...
		Points.Add(PointOnCylinder);
	}
	return Points;
}


TSet<FVector> ULXRFunctionLibrary::GenerateSphereTraceTargets(FVector const& Center, float Radius, int32 Segments)
{
	TSet<FVector> Points;
	// Need at least 4 segments
	Segments = FMath::Max(Segments, 4);

	const float AngleInc = 2.f * UE_PI / Segments;
	int32 NumSegmentsY = Segments;
	float Latitude = AngleInc;
	float SinY1 = 0.0f, CosY1 = 1.0f;

	while (NumSegmentsY--)
	{
		const float SinY2 = FMath::Sin(Latitude);
		const float CosY2 = FMath::Cos(Latitude);

		FVector Vertex1 = FVector(SinY1, 0.0f, CosY1) * Radius + Center;
		FVector Vertex3 = FVector(SinY2, 0.0f, CosY2) * Radius + Center;
		Points.Add(Vertex1);
		Points.Add(Vertex3);
		float Longitude = AngleInc;

		int32 NumSegmentsX = Segments;
		while (NumSegmentsX--)
		{
			const float SinX = FMath::Sin(Longitude);
			const float CosX = FMath::Cos(Longitude);

			const FVector Vertex2 = FVector((CosX * SinY1), (SinX * SinY1), CosY1) * Radius + Center;
			const FVector Vertex4 = FVector((CosX * SinY2), (SinX * SinY2), CosY2) * Radius + Center;
			Points.Add(Vertex2);
			Points.Add(Vertex4);
			Longitude += AngleInc;
		}
		SinY1 = SinY2;
		CosY1 = CosY2;
		Latitude += AngleInc;
	}

	return Points;
}


TSet<FVector> ULXRFunctionLibrary::FilterTargetsByTrace(const UObject* WorldContextObject, const FVector& FilterTraceStartLocation, const TSet<FVector>& TraceTargets, TArray<AActor*> ActorsToIgnore,
                                                        ECollisionChannel TraceChannel)
{
	FCollisionQueryParams Params = FCollisionQueryParams("FilterTargetsByTrace", SCENE_QUERY_STAT_ONLY(KismetTraceUtils), false);
	Params.AddIgnoredActors(ActorsToIgnore);
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	TSet<FVector> OutTargets;
	for (const FVector& End : TraceTargets)
	{
		if (!World->LineTraceTestByChannel(FilterTraceStartLocation, End, TraceChannel, Params))
		{
			OutTargets.Add(End);
		}
	}
	return OutTargets;
}

void ULXRFunctionLibrary::GetLightsInBox(const UObject* WorldContextObject, const FBox& Box, TSet<TWeakObjectPtr<ULXRSourceComponent>>& RelevantLights)
{
	const ULXRSubsystem* LXRSubsystem = WorldContextObject->GetWorld()->GetSubsystem<ULXRSubsystem>();
	TArray<TWeakObjectPtr<ULXRSourceComponent>> AllLights = LXRSubsystem->GetAllLights();
	if (AllLights.Num() > 0)
	{
		for (auto& Light : AllLights)
		{
			if (UKismetMathLibrary::IsPointInBox(Light->GetOwner()->GetActorLocation(), Box.GetCenter(), Box.GetExtent()))
				RelevantLights.Add(Light);
		}
	}

	if (RelevantLights.Num() == 0)
	{
		for (TObjectIterator<ULXRSourceComponent> Itr; Itr; ++Itr)
		{
			if (IsValid(Itr->GetOwner()))
			{
				if (UKismetMathLibrary::IsPointInBox(Itr->GetOwner()->GetActorLocation(), Box.GetCenter(), Box.GetExtent()))
					RelevantLights.Add(*Itr);
			}
		}
	}
}

bool ULXRFunctionLibrary::GetLuxFromTask(const UObject* WorldContextObject, const FTraceTaskData& TraceTaskData, const struct FQueryLXRDebugOptions& QueryLXRDebugOptions, float LXRMultiplier, float MinLuxThreshold)
{
	auto TargetLXRData = TraceTaskData.TargetLXRData;
	return GetLux(WorldContextObject, TraceTaskData, TraceTaskData.PassedDatas, TargetLXRData, QueryLXRDebugOptions, LXRMultiplier, MinLuxThreshold);
}

bool ULXRFunctionLibrary::GetLux(const UObject* WorldContextObject, const FTraceTaskData& TraceTaskData, const TSet<FLightSourcePassedData>& PassedData, FTargetLXRData& TargetLXRData,
                                 const FQueryLXRDebugOptions& QueryLXRDebugOptions, float LXRMultiplier, float MinLuxThreshold)
{
	SCOPE_CYCLE_COUNTER(STAT_GetCombinedDatas);

	// TMap<int, TArray<FLinearColor>> TargetsCombinedLightColors;
	// TMap<int, float> TargetsCombinedLXRIntensity;
	// TMap<int, FLinearColor> IlluminatedTargets;

	TMap<int, TArray<FLinearColor>> TargetsCombinedLightColors;
	TMap<int, TArray<float>> TargetsCombinedLightIntensities;
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	const TArray<FVector>& TraceTargets = TraceTaskData.TraceType == EDetectionType::Silhouette ? TraceTaskData.TracePoints : TraceTaskData.ChosenTracePoints;

	TargetLXRData.Reset();
	TargetLXRData.TraceTargets = TraceTargets;
	int TotalTargets = TraceTaskData.TracePoints.Num();

	for (int i = 0; i < TraceTargets.Num(); ++i)
	{
		TargetsCombinedLightColors.Add(i);
		TargetsCombinedLightIntensities.Add(i);
		// TargetLXRData.AddTargetAndValues(i, 0, FLinearColor::Black);
	}

	TargetLXRData.IlluminatedTargetsCount = 0;
	bool Abort = false;
	for (const auto& Data : PassedData)
	{
		int TotalPassedComponents = Data.PassedComponentsAndTargets.Num();

		if (Abort)
			break;


		if (!Data.SourceComponent.IsValid())
			continue;

		if (!IsValid(Data.SourceComponent->GetOwner()))
			continue;

		TWeakObjectPtr<AActor> LightActor = Data.SourceComponent->GetOwner();
		if (IsValid(LightActor.Get()))
		{
			for (auto& Pair : Data.PassedComponentsAndTargets)
			{
				if (TotalPassedComponents != Data.PassedComponentsAndTargets.Num())
					Abort = true;

				if (Abort)
					break;

				TArray<ULightComponent*> LightComponents = Data.SourceComponent->GetMyLightComponents();

				int CompIndex = Pair.Key;
				TSet<int> PassedTargets = Pair.Value;

				ULightComponent* TargetLight = LightComponents[CompIndex];
				ULocalLightComponent* AsLocalLight = Cast<ULocalLightComponent>(TargetLight);

				float FallOff = 1;
				float Intensity = TargetLight->Intensity;
				float DistancePercent = 0.0f;
				float Attenuation = 0.0f;
				float LuxIntensity = 0;

				if (AsLocalLight)
				{
					Attenuation = AsLocalLight->AttenuationRadius;
					const ELightUnits& IntensityUnits = AsLocalLight->IntensityUnits;
					const float& BeforeConversionIntensity = TargetLight->Intensity;

					float CosHalfConeAngle;
					if (Cast<USpotLightComponent>(TargetLight))
						CosHalfConeAngle = Cast<USpotLightComponent>(TargetLight)->GetCosHalfConeAngle();
					else
						CosHalfConeAngle = -1;

					Intensity = BeforeConversionIntensity * AsLocalLight->GetUnitsConversionFactor(IntensityUnits, ELightUnits::Candelas, CosHalfConeAngle);
				}

				for (const int TargetIndex : PassedTargets)
				{
					if (TraceTaskData.TraceType == EDetectionType::Silhouette)
					{
						if (TraceTaskData.TracePoints.Num() != TotalTargets)
							Abort = true;

						if (!TraceTaskData.TracePoints.IsValidIndex(TargetIndex))
							Abort = true;

						if (Abort)
							break;
					}

					if (!TraceTargets.IsValidIndex(TargetIndex))
						Abort = true;

					if (Abort)
						break;

					const FVector& TargetLocation = TraceTargets[TargetIndex];
					TargetLXRData.AddTarget(TargetIndex);


					float Multiplier = 1;
					float ColorMultiplier = 1;
					switch (TraceTaskData.TraceType)
					{
					case EDetectionType::Detection:
						Multiplier = Data.SourceComponent->LXRMultiplier;
						break;
					case EDetectionType::LightSense:
						Multiplier = Data.SourceComponent->LXRSenseMultiplier;
						break;
					case EDetectionType::Silhouette:
						Multiplier = Data.SourceComponent->LXRSilhouetteMultiplier;
						break;
					case EDetectionType::Environment:
						Multiplier = Data.SourceComponent->LXRMultiplier;
						break;
					default: ;
					}

					{
						if (Data.SourceComponent->LightLXRMultipliers.Num() > 0)
						{
							for (int l = 0; l < Data.SourceComponent->LightLXRMultipliers.Num(); ++l)
							{
								if (Data.SourceComponent->LightLXRMultipliers[l].LightComponent.GetComponent(Data.SourceComponent->GetOwner()) == LightComponents[CompIndex])
								{
									Multiplier *= Data.SourceComponent->LightLXRMultipliers[l].LightData;
									break;
								}
							}
						}
					}


					if (Data.SourceComponent->LightLXRColorMultipliers.Num() == 0)
					{
						ColorMultiplier = Data.SourceComponent->LXRColorMultiplier;
					}
					else
					{
						for (int l = 0; l < Data.SourceComponent->LightLXRColorMultipliers.Num(); ++l)
						{
							if (Data.SourceComponent->LightLXRColorMultipliers[l].LightComponent.GetComponent(Data.SourceComponent->GetOwner()) == LightComponents[CompIndex])
							{
								ColorMultiplier = Data.SourceComponent->LightLXRColorMultipliers[l].LightData;
								break;
							}
						}
					}


					FLinearColor LightColor;

					// Target->LightColor.ComputeAndFixedColorAndIntensity
					if (TargetLight->bUseTemperature)
					{
						LightColor = FLinearColor::MakeFromColorTemperature(TargetLight->Temperature);
						LightColor *= TargetLight->GetLightColor();
					}
					else
						LightColor = TargetLight->GetLightColor();


					const float Distance = FMath::Max(FVector::Distance(TargetLocation, TargetLight->GetComponentLocation()), 0.01f);
					const float DistanceInMeters = Distance / 100.0f;


					if (Cast<UDirectionalLightComponent>(TargetLight))
					{
						LuxIntensity = Intensity;
					}
					else
					{
						DistancePercent = FMath::Abs(Distance / Attenuation - 1);
						if (const USpotLightComponent* SpotLightComponent = Cast<USpotLightComponent>(TargetLight))
						{
							const FVector& SpotForward = TargetLight->GetForwardVector();
							const FVector DirectionToTarget = (TargetLocation - TargetLight->GetComponentLocation()).GetSafeNormal();
							const float Dot = FVector::DotProduct(SpotForward, DirectionToTarget);
							const float Angle = FMath::RadiansToDegrees(FMath::Acos(Dot));

							if (Angle < SpotLightComponent->InnerConeAngle)
							{
								FallOff = 1.0f;
							}
							else if (Angle > SpotLightComponent->OuterConeAngle)
							{
								FallOff = 0.0f;
							}
							else
							{
								FallOff = (SpotLightComponent->OuterConeAngle - Angle) / (SpotLightComponent->OuterConeAngle - SpotLightComponent->InnerConeAngle);
							}
						}
						LuxIntensity = Intensity * FallOff / (DistanceInMeters * DistanceInMeters);
					}

					LightColor.A = FallOff;
					LightColor *= ColorMultiplier;


					TargetsCombinedLightColors[TargetIndex].Add(LightColor);
					TargetsCombinedLightIntensities[TargetIndex].Add(LuxIntensity * Multiplier * LXRMultiplier);
				}
			}
		}
	}

	for (auto& IlluminatedTarget : TargetLXRData.TargetsLXR)
	{
		int Index = IlluminatedTarget.Key;
		float AverageValue = 0;
		float Sum = 0.0f;
		int LuxCountOverMinThreshold = 0;
		float MaxLux = 0.0f;
		TArray<FLinearColor> AverageColorsArray;
		for (int32 i = 0; i < TargetsCombinedLightIntensities[Index].Num(); ++i)
		{
			float Lux = TargetsCombinedLightIntensities[Index][i];
			AverageValue += TargetsCombinedLightIntensities[Index][i];
			AverageColorsArray.Add(TargetsCombinedLightColors[Index][i]);

			if (Lux > MinLuxThreshold)
			{
				Sum += Lux;
				LuxCountOverMinThreshold++;
			}
			MaxLux = FMath::Max(MaxLux, Lux);
		}

		if (LuxCountOverMinThreshold > 0)
		{
			float Average = Sum / LuxCountOverMinThreshold;
			IlluminatedTarget.Value.WeightedIntensity = (0.7f * MaxLux + 0.3f * Average);
		}

		AverageValue /= FMath::Max(TargetsCombinedLightIntensities[Index].Num(), 1);
		FLinearColor AverageColor = ULXRFunctionLibrary::GetLinearColorArrayAverage(TargetsCombinedLightColors[Index]);

		IlluminatedTarget.Value.Intensity = MaxLux;
		IlluminatedTarget.Value.AverageIntensity = AverageValue;

		IlluminatedTarget.Value.Color = AverageColor;

		if (IlluminatedTarget.Value.Intensity > 0)
			TargetLXRData.IlluminatedTargetsCount++;
	}


	if (QueryLXRDebugOptions.bDebugLXR)
	{
		ULXRFunctionLibrary::DebugOnGameThread([World,WorldContextObject,TargetLXRData,QueryLXRDebugOptions]
		{
			DebugLXR(WorldContextObject, TargetLXRData, QueryLXRDebugOptions.DebugLXRDrawTime, QueryLXRDebugOptions.bDebugLXR_OnlyPassed, 8.f);
		});
	}

	return TargetLXRData.IlluminatedTargetsCount > TraceTaskData.RelevantRequiredAmountToPass;
}

void ULXRFunctionLibrary::DebugLXR(const UObject* WorldContextObject, const FTargetLXRData& TargetsLXR, float DebugDrawTime, bool bDebugOnlyPassed,float MaxLux)
{
	if (!IsInGameThread()) return;
	const FLinearColor Red = FColor::Red;
	const FLinearColor Green = FColor::Green;
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	for (auto& IlluminatedTarget : TargetsLXR.TargetsLXR)
	{
		float Intensity = IlluminatedTarget.Value.Intensity;
		float PassedTreshold = 0.25;

		const FVector& Location = TargetsLXR.TraceTargets[IlluminatedTarget.Key];
		float Alpha = Intensity / MaxLux;
		const FLinearColor& Color = IlluminatedTarget.Value.Color*Intensity;
		FLinearColor DebugColor;
		if (Intensity < 0.1f)
		{
			DebugColor = FColor::Black;
		}
		else
		{
			DebugColor = FLinearColor::LerpUsingHSV(Red, Green, Alpha);
		}

		if (bDebugOnlyPassed)
		{
			if (Intensity > 0.1)
			{
				DrawDebugCrosshairs(World, TargetsLXR.TraceTargets[IlluminatedTarget.Key], FRotator::MakeFromEuler(FVector::UpVector), 15.f, Color.ToFColorSRGB(), false, DebugDrawTime, 0);
				DrawDebugPoint(World, Location, 5.f, Color.ToFColorSRGB(), false, DebugDrawTime, 0);
			}
		}
		else
		{
			DrawDebugCrosshairs(World, TargetsLXR.TraceTargets[IlluminatedTarget.Key], FRotator::MakeFromEuler(FVector::UpVector), 15.f, Color.ToFColorSRGB(), false, DebugDrawTime, 0);
			DrawDebugPoint(World, Location, 5.f, Color.ToFColorSRGB(), false, DebugDrawTime, 0);
		}

		if (Intensity > PassedTreshold)
		{
			FText TextIntensity = UKismetTextLibrary::Conv_DoubleToText(Intensity, HalfToEven, false, true, 1, 3, 0, 1);
			DrawDebugString(World, Location, FString::Printf(TEXT("LXR: %s"), *TextIntensity.ToString()),NULL, DebugColor.ToFColorSRGB(), DebugDrawTime, false, 1);
		}
	}
}

void ULXRFunctionLibrary::DebugTargets(const UObject* WorldContextObject, TSet<FVector> Targets, float DrawDebugTime, FLinearColor DebugColor)
{
	UWorld* World = nullptr;
	if (WorldContextObject)
		World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!IsValid(World)) return;

	for (auto& Element : Targets)
	{
		DrawDebugPoint(World, Element, 5.f, DebugColor.ToFColorSRGB(), false, DrawDebugTime);
	}
}

void ULXRFunctionLibrary::BP_GetRelevantLights(const UObject* WorldContextObject, const TArray<FVector>& TraceTargets, TSet<ULXRSourceComponent*>& RelevantLights)
{
	TSet<TWeakObjectPtr<ULXRSourceComponent>> Temp;
	GetRelevantLights(WorldContextObject, TraceTargets, Temp);
	for (auto& Actor : Temp)
	{
		RelevantLights.FindOrAdd(Actor.Get());
	}
}

void ULXRFunctionLibrary::BP_GetLightsInBox(const UObject* WorldContextObject, const FBox& Box, TSet<ULXRSourceComponent*>& RelevantLights)
{
	TSet<TWeakObjectPtr<ULXRSourceComponent>> Temp;
	GetLightsInBox(WorldContextObject, Box, Temp);
	for (auto& Actor : Temp)
	{
		RelevantLights.Add(Actor.Get());
	}
}

void ULXRFunctionLibrary::BP_GetOctreeLights(const UObject* WorldContextObject, const FBox& Box, TSet<ULXRSourceComponent*>& Lights, bool bDebug)
{
	const ULXRSubsystem* LXRSubsystem = WorldContextObject->GetWorld()->GetSubsystem<ULXRSubsystem>();
	FBoxCenterAndExtent BoxCenterAndExtent(Box.GetCenter(), Box.GetExtent());
	TSet<TWeakObjectPtr<ULXRSourceComponent>> TempLights;
	LXRSubsystem->GetOctreeLights(BoxCenterAndExtent, TempLights, bDebug);

	for (auto& Light : TempLights)
	{
		if (Light.IsValid())
		{
			Lights.Add(Light.Get());
		}
	}
}


bool ULXRFunctionLibrary::CheckVisibility(const UObject* WorldContextObject, const FTraceTaskData& TraceTaskData, FLightSourcePassedData& PassedData, const FQueryLXRDebugOptions& QueryLXRDebugOptions)
{
	const TArray<FVector>& TraceTargets = TraceTaskData.TracePoints;

	int PassedChecks = 0;
	UWorld* World = nullptr;
	if (WorldContextObject)
		World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (PassedData.SourceComponent == nullptr)
	{
		return false;
	}

	const TArray<ULightComponent*>& LightComponents = PassedData.SourceComponent->GetMyLightComponents();
	TMap<int, TSet<int>> PassedComponentsAndTargetsToRemove;

	for (auto& Pair : PassedData.PassedComponentsAndTargets)
	{
		const int ThisLoopPassedChecks = PassedChecks;
		if (Pair.Value.IsEmpty()) continue;
		for (const auto& TargetIndex : Pair.Value)
		{
			if (Pair.Key < 0) continue;
			const ULightComponent* LightComponent = LightComponents[Pair.Key];
			const FVector TraceTarget = TraceTargets[TargetIndex];


			const FVector& End = TraceTarget;
			FVector Start;
			// ActorsToIgnore.Append(Cast<ULXRSourceComponent>(LightComponent->GetOwner()->GetComponentByClass(ULXRSourceComponent::StaticClass()))->GetMyOverlappingActors());
			// ActorsToIgnore.Append(Cast<ULXRSourceComponent>(LightComponent->GetOwner()->GetComponentByClass(ULXRSourceComponent::StaticClass()))->GetIgnoreVisibilityActors());


			if (LightComponent->IsA(UDirectionalLightComponent::StaticClass()))
			{
				FVector DirectionalForwardInverse = LightComponent->GetForwardVector() * -1;
				Start = End + DirectionalForwardInverse.GetSafeNormal() * 15000;
			}
			else
			{
				Start = LightComponent->GetComponentLocation();
			}

			FCollisionQueryParams Params("CheckVisibility", SCENE_QUERY_STAT_ONLY(KismetTraceUtils), false);


			TArray<AActor*> ActorsToIgnore;
			// if (WorldContextObject->GetClass()->IsChildOf(AActor::StaticClass()))
			// {
			// 	const AActor* Querier = static_cast<const AActor*>(WorldContextObject);
			// 	ActorsToIgnore.AddUnique(const_cast<AActor*>(Querier));
			// }

			ActorsToIgnore.AddUnique(LightComponent->GetOwner());
			ActorsToIgnore.Append(PassedData.SourceComponent->GetMyOverlappingActors());
			ActorsToIgnore.Append(PassedData.SourceComponent->GetIgnoreVisibilityActors());


			ECollisionChannel ChannelToUse = ECC_MAX;
			if (PassedData.SourceComponent->TraceChannel != ECC_MAX)
			{
				ChannelToUse = PassedData.SourceComponent->TraceChannel;
			}
			else if (TraceTaskData.TraceType == EDetectionType::Detection)
			{
				const FDetectionTaskData* DetectionData = static_cast<const FDetectionTaskData*>(&TraceTaskData);

				if (PassedData.SourceComponent->TraceChannel < ECC_MAX)
					ChannelToUse = PassedData.SourceComponent->TraceChannel;
				else
					ChannelToUse = DetectionData->TraceChannel;


				if (DetectionData->DetectionComponent != nullptr && DetectionData->DetectionComponent->bIgnoreSelfOnVisibilityTraces)
				{
					ActorsToIgnore.Add(DetectionData->DetectionComponent->GetOwner());
				}
			}
			if (ChannelToUse == ECC_MAX)
			{
				ChannelToUse = TraceTaskData.TraceChannel;
			}

			Params.AddIgnoredActors(ActorsToIgnore);


			if (!World->LineTraceTestByChannel(Start, End, ChannelToUse, Params))
			{
				PassedChecks++;
			}
			else
			{
				if (!PassedComponentsAndTargetsToRemove.Contains(Pair.Key))
					PassedComponentsAndTargetsToRemove.Add(Pair.Key);

				if (!PassedComponentsAndTargetsToRemove[Pair.Key].Contains(TargetIndex))
					PassedComponentsAndTargetsToRemove[Pair.Key].Add(TargetIndex);
			}

#if UE_ENABLE_DEBUG_DRAWING
			if (QueryLXRDebugOptions.bDebugVisibility)
			{
				ULXRFunctionLibrary::DebugOnGameThread([QueryLXRDebugOptions, PassedData, World, PassedChecks, Start, End, WeakLightComponent = MakeWeakObjectPtr(LightComponent), Params, ThisLoopPassedChecks]
				{
					if (!WeakLightComponent.IsValid()) return;
					float DebugDrawTime = QueryLXRDebugOptions.DebugVisibilityDrawTime;

					FHitResult HitResult;
					World->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);
					auto TimeSine = (FMath::Sin(World->GetTimeSeconds() * 1.2f) + 2);

					// bool Passed = ThisLoopPassedChecks == PassedChecks;
					bool Passed = !HitResult.bBlockingHit;
					// DrawDebugLine(World, Start, End, ThisLoopPassedChecks == PassedChecks ? FColor::Red : FColor::Green, false, DebugDrawTime, 0, 1);
					// DrawDebugSphere(World, LightComponent->GetComponentLocation(), 15, 12, FColor::Green, false, DebugDrawTime);


					DrawDebugLine(World, End, Passed ? Start : HitResult.Location, Passed ? FColor::Green : FColor::Red, false, 0.1f, 0, 2);
					DrawDebugPoint(World, Start, 15.f, Passed ? FColor::Green : FColor::Red, false, DebugDrawTime, 0);
					DrawDebugDirectionalArrow(World, Start, End, 50.f * TimeSine, Passed ? FColor::Green : FColor::Red, false, DebugDrawTime);

					if (!Passed)
					{
						DrawDebugLine(World, HitResult.Location, Start, FColor::Cyan, false, 0.1f, 0, 1);
						DrawDebugPoint(World, HitResult.Location, 10.f * TimeSine, FColor::Magenta, false, 0.1f);
					}

					// DrawDebugBox(World, result.Location, FVector(10), FColor::Red, false, DebugDrawTime, 0, 2);
					// DrawDebugPoint(World, Start, 15.f, FColor::Red, false, DebugDrawTime, 0);

					if (QueryLXRDebugOptions.bPrintDebug)
						if (HitResult.GetActor())
						{
							if (const auto HitComponent = HitResult.GetComponent())
							{
								UE_LOG(LogLXR, Log, TEXT("Relevancy:: %s:  Hit Actor and component: %s : %s  "), *WeakLightComponent->GetOwner()->GetName(), *HitResult.GetActor()->GetName(), *HitComponent->GetName());
							}
							else
							{
								UE_LOG(LogLXR, Log, TEXT("Relevancy:: %s:  Hit Actor %s  "), *WeakLightComponent->GetOwner()->GetName(), *HitResult.GetActor()->GetName());
							}
						}
				});
			}
#endif
		}
	}
	for (auto& Pair : PassedComponentsAndTargetsToRemove)
	{
		PassedData.RemovePassedTargetsForComponent(Pair.Key, Pair.Value);
	}
	return PassedChecks >= TraceTaskData.RelevantRequiredAmountToPass;
}

// bool ULXRFunctionLibrary::CheckIsLightRelevant(const UObject* WorldContextObject, const ULXRSourceComponent& LightSourceComponent, FLightSourcePassedData& PassedData, const TArray<FVector>& InTraceTargets, const FQueryLXRDebugOptions& QueryLXRDebugOptions)
bool ULXRFunctionLibrary::CheckIsLightRelevant(const UObject* WorldContextObject, const FTraceTaskData& TraceTaskData, FLightSourcePassedData& PassedData, const FQueryLXRDebugOptions& QueryLXRDebugOptions)
{
	TArray<FVector> TraceTargets = TraceTaskData.ChosenTracePoints;
	UWorld* World = nullptr;
	if (WorldContextObject)
		World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!PassedData.SourceComponent.IsValid()) return false;
	TArray<ULightComponent*> LightComponents = PassedData.SourceComponent->GetMyLightComponents();

	PassedData.Clear();

	int PassedChecks = 0;
	int ComponentIdx = 0;
	bool Passed = false;
	bool ForceCheckBecauseOfMemoryState = false;
	const float DebugDrawTime = QueryLXRDebugOptions.DebugRelevancyDrawTime;

	if (const AActor* AsActor = Cast<AActor>(WorldContextObject))
	{
		ULXRMemoryComponent* MemoryComponent = AsActor->FindComponentByClass<ULXRMemoryComponent>();
		if (IsValid(MemoryComponent))
		{
			if ((TraceTaskData.TraceType == EDetectionType::LightSense && MemoryComponent->MemoryCheckClass == EMemoryCheckClass::Sense)
				||
				(TraceTaskData.TraceType == EDetectionType::Detection && MemoryComponent->MemoryCheckClass == EMemoryCheckClass::Detection))
			{
				const ELightState MemorizedState = MemoryComponent->GetStateOfMemorizedLight(PassedData.SourceComponent.Get());
				const ELightState CurrentState = PassedData.SourceComponent->GetLightState();
				ForceCheckBecauseOfMemoryState = MemorizedState != CurrentState;
			}
		}
	}

	for (const ULightComponent* LightComponent : LightComponents)
	{
		for (int i = 0; i < TraceTargets.Num(); ++i)
		{
			bool DoCheck = true;

			if (!ForceCheckBecauseOfMemoryState)
			{
				DoCheck = PassedData.SourceComponent->IsLightComponentEnabled(LightComponent);
			}

			if (!DoCheck)
				continue;

			bool TargetPassed = false;

			const FVector End = TraceTargets[i];
			const FVector Start = LightComponent->GetComponentLocation();


			bool bCheckFailed = false;
			const bool bCheckAllChecks = QueryLXRDebugOptions.bDebugRelevancy;
			const bool bIsDirectionalLight = LightComponent->IsA(UDirectionalLightComponent::StaticClass());
			const bool bIsSpotLight = LightComponent->IsA(USpotLightComponent::StaticClass());
			const bool bIsPointLight = LightComponent->IsA(UPointLightComponent::StaticClass()) && !bIsSpotLight;
			const bool bIsRectLight = LightComponent->IsA(URectLightComponent::StaticClass());

			const bool bCheckDirectional = bIsDirectionalLight;
			const bool bCheckDistance = !bIsDirectionalLight;
			const bool bCheckAttenuation = bIsSpotLight || bIsPointLight || bIsRectLight;
			const bool bCheckDirection = bIsSpotLight || bIsRectLight;
			const bool bCheckIfInsideSpotOrRect = bIsSpotLight || bIsRectLight;


			bool DirectionalPassed = false;
			bool DistancePassed = false;
			bool DirectionPassed = false;
			bool AttenuationPassed = false;
			bool InsideSpotOrRectPassed = false;

			if (bCheckDirectional)
			{
				DirectionalPassed = ULXRFunctionLibrary::CheckDirectionalLight(*LightComponent, TraceTaskData, End, QueryLXRDebugOptions.bDebugRelevancy);


				if (!DirectionalPassed)
					bCheckFailed = true;
			}

			if (bCheckFailed)
			{
				TargetPassed = false;
				if (!bCheckAllChecks)
					continue;
			}

			if (bCheckDistance)
			{
				DistancePassed = ULXRFunctionLibrary::CheckDistance(*PassedData.SourceComponent, TraceTaskData, *LightComponent, Start, End, QueryLXRDebugOptions.bDebugRelevancy);

				// if (DrawDebug)
				// {
				// 	UE_LOG(LogLXR, Verbose, TEXT("Distance to source %s from %s is %f"), *LightSourceComponent.GetOwner()->GetName(), *GetOwner()->GetName(), FVector::Dist(Start,End));
				// }

				if (!DistancePassed)
					bCheckFailed = true;
			}


			if (bCheckFailed)
			{
				TargetPassed = false;
				if (!bCheckAllChecks)
					continue;
			}

			if (bCheckDirection)
			{
				DirectionPassed = ULXRFunctionLibrary::CheckDirection(*LightComponent, TraceTaskData, Start, End, QueryLXRDebugOptions.bDebugRelevancy);
				// if (bDrawDebug && LightSourceComponent.bDrawDebug)
				// 	DrawDebugDirectionalArrow(GetWorld(), End, FMath::Lerp(Start, End, 0.5f), 500, DirectionPassed ? FColor::Magenta : FColor::Cyan, false, DebugDrawTime, 0, 5);

				if (!DirectionPassed)
					bCheckFailed = true;
			}

			//if(!bComponentPassed)
			if (bCheckFailed)
			{
				TargetPassed = false;
				if (!bCheckAllChecks)
					continue;
			}

			if (bCheckAttenuation)
			{
				float AttenuationRadius;

				if (bIsRectLight)
					AttenuationRadius = Cast<URectLightComponent>(LightComponent)->AttenuationRadius;
				else
					AttenuationRadius = Cast<UPointLightComponent>(LightComponent)->AttenuationRadius;

				const float AttenuationSQR = AttenuationRadius * AttenuationRadius;
				AttenuationPassed = CheckAttenuation(AttenuationSQR, TraceTaskData, Start, End, QueryLXRDebugOptions.bDebugRelevancy);

#if UE_ENABLE_DEBUG_DRAWING
				if (QueryLXRDebugOptions.bDebugRelevancy)
				{
					if (IsInGameThread())
						DrawDebugDirectionalArrow(World, Start, End, 250.f, AttenuationPassed ? FColor::Green : FColor::Red, false, DebugDrawTime, 0, 0);
				}
#endif

				if (!AttenuationPassed)
					bCheckFailed = true;
			}

			if (bCheckFailed)
			{
				TargetPassed = false;
				if (!bCheckAllChecks)
					continue;
			}

			if (bCheckIfInsideSpotOrRect)
			{
				InsideSpotOrRectPassed = ULXRFunctionLibrary::CheckIfInsideSpotOrRect(*LightComponent, TraceTaskData, Start, End, bIsSpotLight, QueryLXRDebugOptions.bDebugRelevancy);
				if (!InsideSpotOrRectPassed)
				{
					bCheckFailed = true;
				}
				if (QueryLXRDebugOptions.bDebugRelevancy)
				{
					ULXRFunctionLibrary::DebugOnGameThread([World, Start,DebugDrawTime, End, bCheckFailed]
					{
						DrawDebugDirectionalArrow(World, Start, End, 250.f, !bCheckFailed ? FColor::Yellow : FColor::Green, false, DebugDrawTime, 0, 0);
						DrawDebugCrosshairs(World, Start, FRotator::MakeFromEuler(FVector::UpVector), 20.f, bCheckFailed ? FColor::Red : FColor::Green, false, 0.25f);
					});
				}
			}

#if UE_ENABLE_DEBUG_DRAWING
			if (bCheckFailed && QueryLXRDebugOptions.bDebugRelevancy && QueryLXRDebugOptions.bPrintDebug)
			{
				FString StrDirectionalLightPassed = DirectionalPassed ? TEXT("") : TEXT("DirectionalLight");
				FString StrDistancePassed = DistancePassed ? TEXT("") : TEXT(" Distance");
				FString StrDirectionPassed = DirectionPassed ? TEXT("") : TEXT(" Direction");
				FString StrAttenuationPassed = AttenuationPassed ? TEXT("") : TEXT(" Attenuation");
				FString StrInsideSpotOrRectPassed = InsideSpotOrRectPassed ? TEXT("") : TEXT(" InsideSpotOrRectPassed");

				FString FailedTests;
				if (bIsDirectionalLight)
					FailedTests = StrDirectionalLightPassed;
				if (bIsPointLight)
					FailedTests = StrDistancePassed + StrAttenuationPassed;
				if (bIsSpotLight || bIsRectLight)
					FailedTests = StrDistancePassed + StrDirectionPassed + StrAttenuationPassed + StrInsideSpotOrRectPassed;

				UE_LOG(LogLXR, Warning, TEXT("%s: %s fails checks %s"), *WorldContextObject->GetName(), *PassedData.SourceComponent->GetOwner()->GetName(), *FailedTests)
				TargetPassed = false;
				continue;
			}
#endif
			if (bIsDirectionalLight)
			{
				if (DirectionalPassed)
				{
					TargetPassed = true;
				}
			}
			if (bIsPointLight)
			{
				if (AttenuationPassed && DistancePassed)
				{
					TargetPassed = true;
				}
			}
			if (bIsSpotLight)
			{
				if (AttenuationPassed && DirectionPassed && InsideSpotOrRectPassed && DistancePassed)
				{
					TargetPassed = true;
				}
			}

			if (bIsRectLight)
			{
				if (AttenuationPassed && DirectionPassed && InsideSpotOrRectPassed && DistancePassed)
				{
					TargetPassed = true;
				}
			}

			if (TargetPassed)
			{
				// if(ComponentIdx)
				// {
				PassedChecks++;
				int Index = TraceTaskData.TracePoints.IndexOfByKey(TraceTaskData.ChosenTracePoints[i]);

				PassedData.AddPassedComponentAndTarget(ComponentIdx, Index);


				if (QueryLXRDebugOptions.bDebugRelevancy)
				{
					ULXRFunctionLibrary::DebugOnGameThread([World, Start,DebugDrawTime]
					{
						DrawDebugPoint(World, Start, 5.f, FColor::Magenta, false, DebugDrawTime, 0);
					});
				}
			}
		}


		ComponentIdx++;

		if (PassedChecks >= TraceTaskData.RelevancyRequiredAmountToPass)
			Passed = true;
	}

	// if (!Passed)
	// 	PassedComponents.Empty();

	return Passed;
}

void ULXRFunctionLibrary::QueryLocationsLXR(const UObject* Querier, TSet<FLightSourcePassedData>& PassedData, FTargetLXRData& IlluminatedTargets, TArray<ULXRSourceComponent*> Lights, const TArray<FVector>& Points,
                                            const FQueryLXRDebugOptions& QueryLXRDebugOptions)
{
	PassedData.Reset();
	IlluminatedTargets.Reset();
	TSet<TWeakObjectPtr<ULXRSourceComponent>> LightBatch;
	bool bSoloFound = Querier->GetWorld()->GetSubsystem<ULXRSubsystem>()->bSoloFound;
	bool IsValidLightArray = !Lights.IsEmpty();
	if (Lights.Num() == 1 && IsValidLightArray)
	{
		IsValidLightArray = IsValid(Lights[0]);
	}

	if (!IsValidLightArray)
	{
		GetRelevantLights(Querier, Points, LightBatch);
	}
	else
	{
		for (auto& Actor : Lights)
		{
			LightBatch.Add(Actor);
		}
	}
	FTraceTaskData TraceTaskData = FTraceTaskData(Points);
	TraceTaskData.SetLights(LightBatch.Array());

	TraceTaskData.RelevancyRequiredAmountToPass = 0;
	TraceTaskData.RelevantRequiredAmountToPass = 0;

	for (TWeakObjectPtr<ULXRSourceComponent>& SourceComponent : TraceTaskData.Lights)
	{
		if (!SourceComponent.IsValid()) continue;

		if (!SourceComponent->IsEnabled()) continue;
		if (bSoloFound && !SourceComponent->bSolo) continue;

		FLightSourcePassedData NewData = FLightSourcePassedData(SourceComponent);
		TraceTaskData.PassedDatas.Add(NewData);

		const TArray<ULightComponent*> LightComponents = SourceComponent->GetMyLightComponents();
		if (CheckIsLightRelevant(Querier, TraceTaskData, NewData, QueryLXRDebugOptions))
		{
#if UE_ENABLE_DEBUG_DRAWING
			if (QueryLXRDebugOptions.bDebugRelevancy)
			{
				if (IsInGameThread())
				{
					for (auto& Pair : NewData.PassedComponentsAndTargets)
					{
						for (auto& TraceTargets : Pair.Value)
						{
							const FVector TraceTarget = Points[TraceTargets];

							DrawDebugPoint(Querier->GetWorld(), TraceTarget, 5, FColor::Yellow, false, QueryLXRDebugOptions.DebugRelevancyDrawTime, 0);
						}
					}
				}
			}
#endif
			{
				if (CheckVisibility(Querier, TraceTaskData, NewData, QueryLXRDebugOptions))
				{
					PassedData.Add(NewData);
				}
			}
		}
		if (PassedData.Num() > 0)
		{
			GetLux(Querier, Points, PassedData, IlluminatedTargets, QueryLXRDebugOptions, 1);
		}
	}
}

bool ULXRFunctionLibrary::IsPerceivedActorLit(const AActor* ActorToTest, const float IntensityThreshold)
{
	float LXRIntensity = 0;
	if (const ULXRDetectionComponent* DetectionComponent = ActorToTest->FindComponentByClass<ULXRDetectionComponent>())
	{
		LXRIntensity = DetectionComponent->CombinedLXRIntensity;
	}
	return LXRIntensity > IntensityThreshold;
}

void ULXRFunctionLibrary::FetchSilhouetteTraceParams(const AActor& FromActor, const AActor& ToActor, FSilhouetteTaskData& SilhouetteTaskData)
{
	const ULXRSilhouetteDetectionComponent* SilhouetteDetectionComponent = FromActor.FindComponentByClass<ULXRSilhouetteDetectionComponent>();
	if (!IsValid(SilhouetteDetectionComponent)) return;
	AActor* SilhouetteDetectionComponentOwner = SilhouetteDetectionComponent->GetOwner();

	if (SilhouetteDetectionComponentOwner->GetClass()->ImplementsInterface(ULXRSilhouette::StaticClass()))
	{
		FRotator TempRotator;
		ILXRSilhouette::Execute_GetSilhouetteTraceLocationAndDirection(SilhouetteDetectionComponentOwner, SilhouetteTaskData.InstigatorViewPointLocation, TempRotator);
	}

	if (IsValid(&ToActor))
	{
		FVector Origin;
		if (const auto AsCharacter = Cast<ACharacter>(&ToActor))
		{
			FBoxSphereBounds Bounds;
			if (USkeletalMeshComponent* SkeletalMeshComponent = AsCharacter->GetMesh())
				Bounds = SkeletalMeshComponent->Bounds;
			else
				Bounds = AsCharacter->GetCapsuleComponent()->Bounds;

			Origin = Bounds.Origin;
		}
		else
		{
			FVector Extent;
			constexpr bool OnlyColliding = true;
			ToActor.GetActorBounds(OnlyColliding, Origin, Extent);
		}
		SilhouetteTaskData.SilhouetteLocation = Origin;
		// if (bDrawDebug)
		// {
		// 	DrawDebugBox(GetWorld(), SilhouetteLocation, Extent, FColor::Orange, false, 1.f, 0, 2);
		// }
		// DrawDebugSphere(GetWorld(),SilhouetteLocation,50.f,8,FColor::Yellow,false,1.f);
		SilhouetteTaskData.SilhouetteRotation = UKismetMathLibrary::MakeRotFromX((SilhouetteTaskData.SilhouetteLocation - SilhouetteTaskData.InstigatorViewPointLocation).GetSafeNormal());
	}
}
