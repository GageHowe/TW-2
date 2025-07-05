#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TWPlayerController.generated.h"

UCLASS()
class ATWPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	// Timer for periodic time sync
	FTimerHandle TimeSyncTimerHandle;

	// Configurable sync behavior
	UPROPERTY(EditDefaultsOnly, Category = "TimeSync")
	double SyncInterval = 1.0;

	UPROPERTY(EditDefaultsOnly, Category = "TimeSync")
	int32 MinWarmupSamples = 5;

	UPROPERTY(EditDefaultsOnly, Category = "TimeSync")
	int32 MaxSamples = 15;

	UPROPERTY(EditDefaultsOnly, Category = "TimeSync")
	double TrimFraction = 0.2;

	// Time sync state
	double TimeOffset = 0.0;
	double BestRTT = DBL_MAX;
	double LastClientRequestTime = 0.0;
	int32 SampleCount = 0;
	bool bTimeSyncStable = false;

	// Sample buffer
	TArray<double> OffsetSamples;

	// Sync logic
	void SetupTimeSyncTimer();
	void SyncTimeWithServer();

	// RPCs
	UFUNCTION(Server, Reliable)
	void SR_RequestTime(double clientTime);

	void SR_RequestTime_Implementation(double clientTime);

	UFUNCTION(Client, Reliable)
	void CL_UpdateTime(double serverTime, double clientTime);

	void CL_UpdateTime_Implementation(double serverTime, double clientTime);

public:
	ATWPlayerController();
	double GetServerTime() const;
	void Tick(float DeltaTime);
};
