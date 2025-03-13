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
	UPROPERTY()
	double TimeOffset = 0;
    
	// Round-trip delay (in milliseconds)
	UPROPERTY()
	double RoundTripDelay = 0;

	ATWPlayerController() // default constructor if needed
	{
		TimeOffset = 0.0;
		RoundTripDelay = 0.0;
	}

	void SyncTimeWithServer();

	UFUNCTION(Server, Unreliable)
	void SR_RequestTime(double time);

	UFUNCTION(Client, Unreliable)
	void CL_UpdateTime(double serverTime, double requestTime);
	
	
};
