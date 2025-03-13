#pragma once

#include "CoreMinimal.h"
#include "TestActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "helpers.h"
#include "bthelper.h"
#include "Net/UnrealNetwork.h"
#include "BasicPhysicsPawn.generated.h"

UCLASS()
class BULLETPHYSICSENGINE_API ABasicPhysicsPawn : public APawn
{
	GENERATED_BODY()
public:
	ABasicPhysicsPawn();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	
	FVector DirectionalInput = FVector(0, 0, 0);
	bool PrimaryInput = false;
	bool SecondaryInput = false;

	btRigidBody* MyRigidBody = nullptr;
	
	UPROPERTY(EditAnywhere)
	ATestActor* BulletWorld = nullptr;

	TCircularBuffer<FBulletSimulationState> StateHistoryBuffer = TCircularBuffer<FBulletSimulationState>(128);
	uint32 CurrentHistoryIndex = 0;
	uint32 HistoryCount = 0;
	
	// this is marked false when the pawn should not send or receive input
	// i.e. an inactive vehicle or dead player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	bool IsPossessed = true;

	UFUNCTION()
	void ApplyInputs(const FBulletPlayerInput& input) const;
	void EnableDebug();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerTest();
	UFUNCTION(Server, Reliable)
	void ServerTestSimple();

	UFUNCTION(Server, Unreliable)
	void SendInputsToServer(AActor* actor, FBulletPlayerInput input);

	void resim(FBulletSimulationState state)
	{
		// auto time = state.FrameNumber;
		// auto x = AGameStateBase::GetServerWorldTimeSeconds;
		// // for (???)
		// // {
		// // 	this->ApplyInputs(LocalInputBuffer[???]);
		// // 	everythingelse->ApplyInputs(MostRecentInput);
		// // 	BulletWorld->StepPhysics((1.0f/60.0f), 0);
		// // }
		// FBulletSimulationState newState = state;
		// BulletWorld->SetState(newState);
	}
	
	// Simple helper methods
	void AddHistoryEntry(const FBulletSimulationState& Entry)
	{
		StateHistoryBuffer[CurrentHistoryIndex] = Entry;
		CurrentHistoryIndex = StateHistoryBuffer.GetNextIndex(CurrentHistoryIndex);
		HistoryCount = FMath::Min(HistoryCount + 1, (uint32)StateHistoryBuffer.Capacity());
	}

	const FBulletSimulationState* FindHistoryEntryByFrame(int32 FrameNumber) const
	{
		if (HistoryCount == 0) return nullptr;
    
		uint32 Index = CurrentHistoryIndex;
		for (uint32 i = 0; i < HistoryCount; ++i)
		{
			Index = StateHistoryBuffer.GetPreviousIndex(Index);
			if (StateHistoryBuffer[Index].FrameNumber == FrameNumber)
				return &StateHistoryBuffer[Index];
		}
		return nullptr;
	}
	const FBulletSimulationState* GetLatestHistoryEntry() const
	{
		if (HistoryCount == 0)
			return nullptr;
		return &StateHistoryBuffer[StateHistoryBuffer.GetPreviousIndex(CurrentHistoryIndex)];
	}
	// void ATestActor::PopulateHistoryBuffer()
	// {
	// 	if (bResimulating)
	// 		return;
 //        
	// 	FBulletSimulationState NewEntry;
	// 	NewEntry.FrameNumber = CurrentFrameNumber;
	// 	NewEntry.SimState = GetCurrentState();
 //    
	// 	// Collect inputs from all players
	// 	for (const auto& Pair : ActorToBody)
	// 	{
	// 		AActor* Actor = Pair.Key;
	// 		if (LastAppliedInputs.Contains(Actor))
	// 		{
	// 			NewEntry.PlayerInputs.Add(Actor, LastAppliedInputs[Actor]);
	// 		}
	// 	}
 //    
	// 	AddHistoryEntry(NewEntry);
	// }

private:
	UPROPERTY(EditDefaultsOnly)
	UStaticMeshComponent* StaticMesh;

	UPROPERTY(EditDefaultsOnly)
	UCameraComponent* Camera;
protected:
	UPROPERTY(EditDefaultsOnly)
	ATestActor* world;

	// set inputs on axis event
	void SetForwardInput(float Value) { DirectionalInput.X = Value; }
	void SetRightInput(float Value) { DirectionalInput.Y = Value; }
	void SetUpInput(float Value) { DirectionalInput.Z = Value; }
};

