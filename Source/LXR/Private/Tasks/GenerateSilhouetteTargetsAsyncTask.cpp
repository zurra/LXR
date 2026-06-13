// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "Tasks/GenerateSilhouetteTargetsAsyncTask.h"

#include "LXRDetectionComponent.h"
#include "LXR_Shared.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "MathUtil.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Tasks/TraceTask/Data/FSilhouetteTaskData.h"
#include "DrawDebugHelpers.h"
#include "KismetTraceUtils.h"


void FGenerateSilhouetteTargetsAsyncTask::DoWork()
{
	TaskData->StartState();
	GenerateSilhouetteTargets();
	TaskData->TargetIterator = TaskData->TracePoints.Num() - 1;
	TaskData->NextState();

}


void FGenerateSilhouetteTargetsAsyncTask::GenerateSilhouetteTargets()
{
	TaskData->TracePoints.Reset();
	TRACE_CPUPROFILER_EVENT_SCOPE_STR("GenerateSilhouetteTargets");

	auto Round = [](const FVector& V) -> FVector
	{
		return FVector(
			FMathd::Round(V[0]),
			FMathd::Round(V[1]),
			FMathd::Round(V[2]));
	};
	


	bool bDrawDebug = TaskData->bDrawDebug;
	const FVector& Start = TaskData->SilhouetteLocation;
	const FVector& OwnerViewPointLocation = TaskData->InstigatorViewPointLocation;
	FVector End;

	AActor* SilhouetteActor = TaskData->DetectionComponent->GetOwner();
	FBoxSphereBounds& Bounds = TaskData->Bounds;

	int& SilhouetteEdges = TraceTaskData->SilhouetteEdges;
	int& SilhouetteDistance = TraceTaskData->SilhouetteDistance;
	int& DistancePerSegment = TraceTaskData->DistancePerSegment;
	const int Segments = FMath::RoundToInt(static_cast<float>(SilhouetteDistance) / 250);

	
	FCollisionQueryParams Params = FCollisionQueryParams(TaskData->Instigator->GetFName(), SCENE_QUERY_STAT_ONLY(KismetTraceUtils), false);
	Params.AddIgnoredActor(TaskData->Instigator);
	Params.AddIgnoredActor(TaskData->DetectionComponent->GetOwner());

	FHitResult HitResult;
	bool TraceHit;

	TArray<FLinearColor> DebugColors;
	if (bDrawDebug)
	{
		DebugColors.Add(FColor::Red);
		DebugColors.Add(FColor::Green);
		DebugColors.Add(FColor::Blue);
		DebugColors.Add(FColor::Yellow);
		DebugColors.Add(FColor::Cyan);
		DebugColors.Add(FColor::Magenta);
		DebugColors.Add(FColor::Purple);
		DebugColors.Add(FColor::White);
	}

	if (TaskData->DetectionComponent->IsSocketsEnabled())
	{
		TArray<FVector> SocketTargets = TaskData->DetectionComponent->GetTraceTargets(true, ETraceTarget::Sockets);
		for (auto& SocketTarget : SocketTargets)
		{
			constexpr int SilhouetteInnerTracesCount = 4;
			for (int Count = 0; Count < SilhouetteInnerTracesCount; ++Count)
			{
				float Mpl = static_cast<float>(Count) / static_cast<float>(SilhouetteInnerTracesCount);
				FVector StartToUse = SocketTarget + (Start - SocketTarget) * static_cast<double>(Mpl);
		
				if (bDrawDebug)
				{
					DrawDebugPoint(GetWorld(), StartToUse, 15.f, FColor::Purple, false, 1, 0);
				}
		
		
				FVector DirectionFromOwnerViewPointToTarget = (StartToUse - OwnerViewPointLocation).GetSafeNormal();
				FRotator RotatorFromEdgeToOwnerViewPoint = UKismetMathLibrary::MakeRotFromX(DirectionFromOwnerViewPointToTarget);
		
				End = (StartToUse + DirectionFromOwnerViewPointToTarget * SilhouetteDistance);
				// DrawDebugPoint(GetWorld(), End, 15.f, FColor::Red, false, 1, 0);
		
		
				TraceHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartToUse + (DirectionFromOwnerViewPointToTarget * -10), End, ECC_Visibility, Params);
				if (TraceHit)
				{
					End = HitResult.ImpactPoint;
					// GeneratedTargets.Add(Round(HitResult.ImpactPoint));
					// FVector Direction = (HitResult.TraceStart - HitResult.TraceEnd).GetSafeNormal();
					FVector PointToAdd = HitResult.ImpactPoint + (DirectionFromOwnerViewPointToTarget * -1) * 50;
					float DistanceToSilhouetteActor = (SilhouetteActor->GetActorLocation() - PointToAdd).Length();
					if (DistanceToSilhouetteActor > 200)
						TaskData->TracePoints.Add(PointToAdd);
				}
		
				if (bDrawDebug)
				{
				 	FLinearColor DebugColor = DebugColors[Count];
				//
				 	DrawDebugLine(GetWorld(), StartToUse, End, DebugColor.ToFColorSRGB(), false, 1, 0, 0);
					
					// DrawDebugLineTraceSingle(GetWorld(), StartToUse + (DirectionFromOwnerViewPointToEdge * -10), End, EDrawDebugTrace::ForDuration, TraceHit, HitResult, DebugColors[Count], FLinearColor::Red, 0.25f);
				
				 }
		
				for (int Segment = 1; Segment < Segments; ++Segment)
				{
					int IteratorDistance = Segment * DistancePerSegment;
		
					float DistanceToAdd = FMath::Wrap<float>(IteratorDistance + GetWorld()->GetRealTimeSeconds() * 150, 100.f, SilhouetteDistance * FMath::Max(HitResult.Time, 0.1f));
		
					// if(bDrawDebug)
					// 	DrawDebugLineTraceSingle(GetWorld(), CylinderEdge + (Rotator.Vector() * -50), End, EDrawDebugTrace::ForDuration, TraceHit, HitResult, DebugColors[Count], FLinearColor::Red, GetComponentTickInterval());
		
					if (DistanceToAdd > (StartToUse-End).Length() * HitResult.Time) continue;
		
					FVector TracePoint = FVector::ForwardVector * static_cast<double>(DistanceToAdd);
					float CurrentDistancePercentFromSenseDistance = DistanceToAdd / SilhouetteDistance;
		
		
					bool UseTracePoint = UKismetMathLibrary::RandomBoolWithWeight(FMath::Max(0.1f, CurrentDistancePercentFromSenseDistance * 1));
					if (UseTracePoint)
					{
						TRACE_CPUPROFILER_EVENT_SCOPE_STR("GenerateSilhouetteTargets: UseTracePoint");
		
						FVector TransformedTracePoint = RotatorFromEdgeToOwnerViewPoint.RotateVector(TracePoint) + StartToUse;
		
						if (bDrawDebug)
						{
							DrawDebugPoint(GetWorld(), TransformedTracePoint, 8, FColor::Red, false, .25, 0);
						}
		
						TraceHit = GetWorld()->LineTraceSingleByChannel(HitResult, TransformedTracePoint, TransformedTracePoint + FVector::DownVector * Bounds.BoxExtent.Z * 2, ECC_Visibility, Params);
						if (TraceHit)
						{
							FVector PointToAdd = HitResult.ImpactPoint + FVector::UpVector * 25;
							float DistanceToSilhouetteActor = (SilhouetteActor->GetActorLocation() - PointToAdd).Length();
							if (DistanceToSilhouetteActor > 200)
							{
								TaskData->TracePoints.Add(PointToAdd);
								// DrawDebugPoint(GetWorld(), PointToAdd, 15.f, FColor::Blue, false, .5, 0);
							}
						}
		
						// if(bDrawDebug)
						// 	DrawDebugLineTraceSingle(GetWorld(), TransformedTracePoint, TransformedTracePoint + FVector::DownVector * Bounds.BoxExtent.Z * 2, EDrawDebugTrace::ForDuration, TraceHit, HitResult, FColor::Green, FLinearColor::Red, 0.25);
		
						// if (bDrawDebug)
						// {
						// 	FLinearColor DebugColor = DebugColors[Count];
						//
						// 	DrawDebugLine(GetWorld(), StartToUse, End, DebugColor.ToFColorSRGB(), false, .1, 0, 0);
						// 	DrawDebugPoint(GetWorld(), End, 15.f, DebugColor.ToFColorSRGB(), false, .1, 0);
						// }
					}
				}
			}
		}
		if (bDrawDebug)
		{
			for (auto& Point : TaskData->TracePoints)
			{
				DrawDebugPoint(GetWorld(), Point, 5, FColor::Yellow, false, .5, 0);
			}
		}
	}
	else
	{
		TArray<FVector> CylinderEdges;

		// Calculate the rotation matrix based on the direction
		UE::Math::TMatrix<double> RotationMatrix = FRotationMatrix::MakeFromX(TaskData->SilhouetteRotation.RotateVector(FVector::RightVector));

		// Calculate the axis vector of the cylinder
		const FVector& AxisVector = FVector::ForwardVector;

		// if (auto AsCharacter = Cast<ACharacter>(SilhouetteActo))
		// {
		// 	UCapsuleComponent* CapsuleComponent = AsCharacter->GetCapsuleComponent();
		// 	Bounds = AsCharacter->GetCapsuleComponent()->Bounds;
		// }


		if (auto AsCharacter = Cast<ACharacter>(SilhouetteActor))
		{
			USkeletalMeshComponent* SkeletalMeshComponent = AsCharacter->GetMesh();
			if (SkeletalMeshComponent)
				Bounds = SkeletalMeshComponent->Bounds;
			else
				Bounds = AsCharacter->GetCapsuleComponent()->Bounds;
		}
		else
		{
			FVector Origin, Extent;
			FBoxCenterAndExtent BoxCenterAndExtent;
			SilhouetteActor->GetActorBounds(true, Origin, Extent);
			BoxCenterAndExtent.Center = Origin;
			BoxCenterAndExtent.Extent = Extent;
			Bounds = FBoxSphereBounds(BoxCenterAndExtent.GetBox());
		}


		for (int32 SideIndex = 0; SideIndex < SilhouetteEdges; ++SideIndex)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE_STR("GenerateSilhouetteTargets: Cylinder");
			// Calculate the angle for the current side
			double Theta = 360.0f * (static_cast<float>(SideIndex) / static_cast<float>(SilhouetteEdges));

			// Convert angle to radians
			double ThetaRad = FMath::DegreesToRadians(Theta + GetWorld()->RealTimeSeconds * 150);

			// Calculate the position on the cylinder
			FVector PointOnCylinder = Start + RotationMatrix.TransformVector(AxisVector * Bounds.BoxExtent.Y * FMath::Cos(ThetaRad))
				+ RotationMatrix.GetUnitAxis(EAxis::Z) * Bounds.BoxExtent.Z * FMath::Sin(ThetaRad);

			// Do something with the PointOnCylinder, such as spawning a mesh or drawing a debug point
			// ...
			CylinderEdges.AddUnique(PointOnCylinder);
		}



		if (bDrawDebug)
		{
			for (int c = 0; c < CylinderEdges.Num(); ++c)
			{
				float Wrapped = FMath::WrapExclusive(c + 1, 0, CylinderEdges.Num());
				FVector C1 = CylinderEdges[c];
				FVector C2 = CylinderEdges[Wrapped];

				UKismetMathLibrary::MakeRotFromX(FVector::ForwardVector);
				const FVector Direction2D = TaskData->SilhouetteRotation.Vector().GetSafeNormal2D();

				float AngleC1 = UKismetMathLibrary::Vector_HeadingAngle(Direction2D);
				float AngleC2 = UKismetMathLibrary::Vector_HeadingAngle(Direction2D);

				// Define the original range of the heading angle [-pi, pi]
				const float OriginalMin = -PI;
				const float OriginalMax = PI;

				// Define the desired range [-1, 1]
				const float DesiredMax = 180.f;
				const float DesiredMin = -DesiredMax;

				// Map the angle from the original range to the desired range
				AngleC1 = DesiredMin + (DesiredMax - DesiredMin) * (AngleC1 - OriginalMin) / (OriginalMax - OriginalMin);

				C1 = UKismetMathLibrary::RotateAngleAxis(Start - C1, AngleC1, FVector::UpVector) + Start;
				C2 = UKismetMathLibrary::RotateAngleAxis(Start - C2, AngleC1, FVector::UpVector) + Start;
				GEngine->AddOnScreenDebugMessage(95, 1, FColor::Magenta, FString::Printf(TEXT("Angles | %f | %f"), AngleC1, AngleC2));
				DrawDebugLine(GetWorld(), C1, C2, FColor::Emerald, false, 0.1, 0, 5);
			}
		}




		int EdgeIterator = 0;

		for (auto& CylinderEdge : CylinderEdges)
		{
			constexpr int SilhouetteInnerTracesCount = 4;
			for (int Count = 0; Count < SilhouetteInnerTracesCount; ++Count)
			{
				float Mpl = static_cast<float>(Count) / static_cast<float>(SilhouetteInnerTracesCount);
				FVector StartToUse = CylinderEdge + (Start - CylinderEdge) * static_cast<double>(Mpl);

				if (bDrawDebug)
				{
					DrawDebugPoint(GetWorld(), StartToUse, 15.f, FColor::Purple, false, 1, 0);
				}


				FVector DirectionFromOwnerViewPointToEdge = (StartToUse - OwnerViewPointLocation).GetSafeNormal();
				FRotator RotatorFromEdgeToOwnerViewPoint = UKismetMathLibrary::MakeRotFromX(DirectionFromOwnerViewPointToEdge);


				int NextCorner = FMath::WrapExclusive(EdgeIterator + 1, 0, CylinderEdges.Num());
				FVector NextEdgeCorner = CylinderEdges[NextCorner];
				End = (StartToUse + DirectionFromOwnerViewPointToEdge * SilhouetteDistance) + (NextEdgeCorner - CylinderEdge) * 4;

				DrawDebugCrosshairs(GetWorld(), End, FRotator::ZeroRotator, 15.f, FColor::Cyan, false, 1, 0);


				TraceHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartToUse + (DirectionFromOwnerViewPointToEdge * -10), End, ECC_Visibility, Params);
				if (TraceHit)
				{
					End = HitResult.ImpactPoint;
					// GeneratedTargets.Add(Round(HitResult.ImpactPoint));
					// FVector Direction = (HitResult.TraceStart - HitResult.TraceEnd).GetSafeNormal();
					FVector PointToAdd = HitResult.ImpactPoint + (DirectionFromOwnerViewPointToEdge * -1) * 50;
					float DistanceToSilhouetteActor = (SilhouetteActor->GetActorLocation() - PointToAdd).Length();
					if (DistanceToSilhouetteActor > 200)
						TaskData->TracePoints.Add(PointToAdd);
				}

				if (bDrawDebug)
				{
				 	FLinearColor DebugColor = DebugColors[Count];
				//
				 	DrawDebugLine(GetWorld(), StartToUse, End, DebugColor.ToFColorSRGB(), false, 1, 0, 0);
					
					// DrawDebugLineTraceSingle(GetWorld(), StartToUse + (DirectionFromOwnerViewPointToEdge * -10), End, EDrawDebugTrace::ForDuration, TraceHit, HitResult, DebugColors[Count], FLinearColor::Red, 0.25f);
				
				 }

				for (int Segment = 1; Segment < Segments; ++Segment)
				{
					int IteratorDistance = Segment * DistancePerSegment;

					float DistanceToAdd = FMath::Wrap<float>(IteratorDistance + GetWorld()->GetRealTimeSeconds() * 150, 100.f, SilhouetteDistance * FMath::Max(HitResult.Time, 0.1f));

					// if(bDrawDebug)
					// 	DrawDebugLineTraceSingle(GetWorld(), CylinderEdge + (Rotator.Vector() * -50), End, EDrawDebugTrace::ForDuration, TraceHit, HitResult, DebugColors[Count], FLinearColor::Red, GetComponentTickInterval());

					if (DistanceToAdd > SilhouetteDistance * HitResult.Time) continue;

					FVector TracePoint = FVector::ForwardVector * static_cast<double>(DistanceToAdd);
					float CurrentDistancePercentFromSenseDistance = DistanceToAdd / SilhouetteDistance;


					bool UseTracePoint = UKismetMathLibrary::RandomBoolWithWeight(FMath::Max(0.1f, CurrentDistancePercentFromSenseDistance * 1));
					if (UseTracePoint)
					{
						TRACE_CPUPROFILER_EVENT_SCOPE_STR("GenerateSilhouetteTargets: UseTracePoint");

						FVector TransformedTracePoint = RotatorFromEdgeToOwnerViewPoint.RotateVector(TracePoint) + StartToUse;

						if (bDrawDebug)
						{
							DrawDebugPoint(GetWorld(), TransformedTracePoint, 8, FColor::Red, false, .25, 0);
						}

						TraceHit = GetWorld()->LineTraceSingleByChannel(HitResult, TransformedTracePoint, TransformedTracePoint + FVector::DownVector * Bounds.BoxExtent.Z * 2, ECC_Visibility, Params);
						if (TraceHit)
						{
							FVector PointToAdd = HitResult.ImpactPoint + FVector::UpVector * 25;
							float DistanceToSilhouetteActor = (SilhouetteActor->GetActorLocation() - PointToAdd).Length();
							if (DistanceToSilhouetteActor > 200)
							{
								TaskData->TracePoints.Add(PointToAdd);
								// DrawDebugPoint(GetWorld(), PointToAdd, 15.f, FColor::Blue, false, .5, 0);
							}
						}

						// if(bDrawDebug)
						// 	DrawDebugLineTraceSingle(GetWorld(), TransformedTracePoint, TransformedTracePoint + FVector::DownVector * Bounds.BoxExtent.Z * 2, EDrawDebugTrace::ForDuration, TraceHit, HitResult, FColor::Green, FLinearColor::Red, 0.25);

						// if (bDrawDebug)
						// {
						// 	FLinearColor DebugColor = DebugColors[Count];
						//
						// 	DrawDebugLine(GetWorld(), StartToUse, End, DebugColor.ToFColorSRGB(), false, .1, 0, 0);
						// 	DrawDebugPoint(GetWorld(), End, 15.f, DebugColor.ToFColorSRGB(), false, .1, 0);
						// }
					}
				}
			}

			EdgeIterator++;
		}
		if (bDrawDebug)
		{
			for (auto& Point : TaskData->TracePoints)
			{
				DrawDebugPoint(GetWorld(), Point, 5, FColor::Yellow, false, .5, 0);
			}
		}
	}
}

const UWorld* FGenerateSilhouetteTargetsAsyncTask::GetWorld() const
{
	return TaskData->Instigator->GetWorld();
}

TStatId FGenerateSilhouetteTargetsAsyncTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FLightSenseTraceTargetAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
}

void FGenerateSilhouetteTargetsAsyncTask::FilterGeneratedTargets()
{
	// FCollisionQueryParams Params = FCollisionQueryParams(TaskData->SilhouetteDetectionComponent->GetOwner()->GetFName(), SCENE_QUERY_STAT_ONLY(KismetTraceUtils), false);
	// Params.AddIgnoredActor(TaskData->SilhouetteDetectionComponent->GetOwner());
	// if (IsValid(TaskData->DetectionComponent->GetOwner()))
	// 	Params.AddIgnoredActor(TaskData->SilhouetteDetectionComponent->GetOwner());
	//
	// TArray<FVector> OutTargets;
	// FVector StartLocation = TaskData->SilhouetteLocation
	//
	// for (int i = GeneratedTargets.Num() - 1; i >= 0; --i)
	// {
	// 	if (!GetWorld()->LineTraceTestByChannel(StartLocation, End, TraceChannel, Params))
	// 	{
	// 		OutTargets.Add(End);
	// 		DrawDebugBox(GetWorld(), End, FVector(5), FColor::Green, false, 0.1f, 0, 2);
	// 	}
	// 	else
	// 	{
	// 		DrawDebugBox(GetWorld(), End, FVector(5), FColor::Orange, false, 0.1f, 0, 2);
	// 	}
	// }
	// for (const FVector& End : GeneratedTargets)
	// {
	//
	// 	INC_DWORD_STAT(STAT_TRACESILHOUETTETARGETS);
	// 	if (!GetWorld()->LineTraceTestByChannel(StartLocation, End, TraceChannel, Params))
	// 	{
	// 		OutTargets.Add(End);
	// 		DrawDebugBox(GetWorld(), End, FVector(5), FColor::Green, false, 0.1f, 0, 2);
	// 	}
	// 	else
	// 	{
	// 		DrawDebugBox(GetWorld(), End, FVector(5), FColor::Orange, false, 0.1f, 0, 2);
	// 	}
	// }
	//
	// SilhouetteComponent->TraceTargetsFromThread(OutTargets);
}
