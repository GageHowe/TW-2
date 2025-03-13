// Fill out your copyright notice in the Description page of Project Settings.

// this is the player's player controller for ThreadWraith.

#include "TWPlayerController.h"

void ATWPlayerController::SyncTimeWithServer()
{
	if (!HasAuthority())
	{
		auto localTime = GetWorld()->TimeSeconds;
		SR_RequestTime(localTime);
	} else {
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Uh, you just called SyncTimeWithServer() from the server"));
	}
}

void ATWPlayerController::SR_RequestTime_Implementation(double requestTime)
{
	double serverTime = GetWorld()->TimeSeconds;
	CL_UpdateTime(serverTime, requestTime);
}

void ATWPlayerController::CL_UpdateTime_Implementation(double serverTime, double requestTime)
{
	double localTime = GetWorld()->TimeSeconds;
	RoundTripDelay = localTime - requestTime;
	TimeOffset = RoundTripDelay / 2;

	// sync client clock
	double lastTime = GetWorld()->TimeSeconds;
	GetWorld()->TimeSeconds = serverTime + TimeOffset;

	// logging
	
	// GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, 
	// FString::Printf(TEXT("RoundTrip: %f, Half: %f, clock set to: %f, was %f"), 
	// RoundTripDelay, TimeOffset, GetWorld()->TimeSeconds, lastTime));
	// UE_LOG(LogTemp, Warning, TEXT("RoundTrip: %f, Half: %f, clock set to: %f, was %f"), 
	// RoundTripDelay, TimeOffset, GetWorld()->TimeSeconds, lastTime);
}
