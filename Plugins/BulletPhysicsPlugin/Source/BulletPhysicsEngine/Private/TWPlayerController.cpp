// Fill out your copyright notice in the Description page of Project Settings.

// this is the player's player controller for ThreadWraith.

#include "TWPlayerController.h"
#include "HAL/PlatformFilemanager.h"

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
	double offset = serverTime - clientTime - (roundTrip * 0.5);
	
	// Simple moving average with last few samples
	OffsetSamples.Add(offset);
	if (OffsetSamples.Num() > 5)
	{
		OffsetSamples.RemoveAt(0);
	}
	
	// Calculate average
	double sum = 0.0;
	for (double sample : OffsetSamples)
	{
		sum += sample;
	}
	TimeOffset = sum / OffsetSamples.Num();
}

double ATWPlayerController::GetServerTime() const
{
	return FPlatformTime::Seconds() + TimeOffset;
}