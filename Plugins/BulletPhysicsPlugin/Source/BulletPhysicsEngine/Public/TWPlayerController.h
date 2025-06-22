// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TWPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class BULLETPHYSICSENGINE_API ATWPlayerController : public APlayerController
{
	GENERATED_BODY()
public:

	virtual void BeginPlay() override;
	
	UPROPERTY()
	double TimeOffset = 0;
	UPROPERTY()
	double RoundTripDelay = 0;

	FTimerHandle TimeSyncTimerHandle;
	void SetupTimeSyncTimer();

	ATWPlayerController() // default constructor if needed
	{
		TimeOffset = 0.0;
		RoundTripDelay = 0.0;
	}

	UPROPERTY()
	TArray<double> OffsetSamples;
	
	void SyncTimeWithServer();
	
	// Get current server time
	UFUNCTION(BlueprintCallable)
	double GetServerTime() const;

	// Update RPC signatures  
	UFUNCTION(Server, Reliable)
	void SR_RequestTime(double clientTime);
	
	UFUNCTION(Client, Reliable)
	void CL_UpdateTime(double serverTime, double clientTime);
};
