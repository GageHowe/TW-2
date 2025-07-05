// Fill out your copyright notice in the Description page of Project Settings.

// this is the player's player controller for ThreadWraith.

#include "TWPlayerController.h"
#include "HAL/PlatformFilemanager.h"
#include "Algo/Sort.h"

ATWPlayerController::ATWPlayerController()
{
	// Optionally initialize variables here
	TimeOffset = 0.0;
	SampleCount = 0;
	bTimeSyncStable = false;
	BestRTT = DBL_MAX;
	LastClientRequestTime = 0.0;
}

void ATWPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetupTimeSyncTimer();
}

void ATWPlayerController::SetupTimeSyncTimer()
{
	if (!HasAuthority())
	{
		GetWorldTimerManager().SetTimer(TimeSyncTimerHandle, this, &ATWPlayerController::SyncTimeWithServer, 1.0f, true);
	}
}

void ATWPlayerController::SyncTimeWithServer()
{
	if (!HasAuthority())
	{
		double clientTime = FPlatformTime::Seconds();
		SR_RequestTime(clientTime);
	}
}

void ATWPlayerController::SR_RequestTime_Implementation(double clientTime)
{
	double serverTime = FPlatformTime::Seconds();
	CL_UpdateTime(serverTime, clientTime);
}

void ATWPlayerController::CL_UpdateTime_Implementation(double serverTime, double clientTime)
{
	double currentTime = FPlatformTime::Seconds();
	double roundTrip = currentTime - clientTime;

	// Only accept round trips that are close to the best seen
	if (roundTrip < BestRTT * 1.2)
	{
		if (roundTrip < BestRTT)
		{
			BestRTT = roundTrip;
		}

		double offset = serverTime - clientTime - (roundTrip * 0.5);

		// Add sample to sliding window
		OffsetSamples.Add(offset);
		if (OffsetSamples.Num() > 15)
		{
			OffsetSamples.RemoveAt(0);
		}

		// Trimmed mean calculation
		TArray<double> SortedSamples = OffsetSamples;
		Algo::Sort(SortedSamples);

		int NumSamples = SortedSamples.Num();
		int Trim = FMath::FloorToInt(NumSamples * 0.2); // 20% trimming
		int Start = FMath::Clamp(Trim, 0, NumSamples - 1);
		int End = FMath::Clamp(NumSamples - Trim, Start + 1, NumSamples);

		double Sum = 0.0;
		for (int i = Start; i < End; ++i)
		{
			Sum += SortedSamples[i];
		}

		double NewOffset = Sum / (End - Start);

		// EWMA smoothing for stability (adjust factor as needed)
		constexpr double SmoothingFactor = 0.1;
		TimeOffset = (1.0 - SmoothingFactor) * TimeOffset + SmoothingFactor * NewOffset;
	}
}


double ATWPlayerController::GetServerTime() const
{
	return FPlatformTime::Seconds() + TimeOffset;
}

void ATWPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("ACTUAL: %.6f"), FPlatformTime::Seconds());
	} else
	{
		UE_LOG(LogTemp, Warning, TEXT("PREDICTED: %.6f"), GetServerTime());
	}
}
