#pragma once

#include "CoreMinimal.h"
#include "TestActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "helpers.h"
#include "BasicPhysicsPawn.generated.h"

UCLASS()
class BULLETPHYSICSENGINE_API ABasicPhysicsPawn : public APawn
{
	GENERATED_BODY()
public:
	ABasicPhysicsPawn();
	virtual void Tick(float DeltaTime) override;
	virtual void AsyncPhysicsTickActor(float DeltaTime, float SimTime) override;

	void SetTurnUpInput(float X)
	{
		CurrentTurnUp = X;
		AddControllerPitchInput(X);
	}

	void SetTurnRightInput(float X)
	{
		CurrentTurnRight = X;
	}


	void SetRollRightInput(float X)
	{
		CurrentRollRight = X;
	}
	
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	
	FVector CurrentDirectionalInput = FVector(0, 0, 0);
	bool CurrentPrimaryInput = false;
	bool CurrentSecondaryInput = false;
	bool CurrentBoostInput = false;
	float CurrentTurnRight = 0;
	float CurrentTurnUp = 0;
	float CurrentRollRight = 0;

	bool mustCorrectState = false;
	
	btRigidBody* MyRigidBody = nullptr;
	
	UPROPERTY(EditAnywhere)
	ATestActor* BulletWorld = nullptr;

	// DEPRECATED MAYBE
	// this is marked false when the pawn should not send or receive input
	// i.e. an inactive vehicle or dead player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	bool IsPossessed = true;

	UFUNCTION()
	virtual void ApplyInputs(const FTWPlayerInput& input); // virtual keyword needs to be present to make a function overridable
	
	UFUNCTION(Server, Reliable)
	void ServerTestSimple();

	UFUNCTION(Server, Unreliable)
	void SendInputsToServer(AActor* actor, FTWPlayerInput input);
	
private:
	UPROPERTY(EditDefaultsOnly)
	UStaticMeshComponent* StaticMesh;

	UPROPERTY(EditDefaultsOnly)
	UCameraComponent* Camera;
protected:
	UPROPERTY(EditDefaultsOnly)
	ATestActor* world;

	// Inputs
	void SetForwardInput(float Value) { CurrentDirectionalInput.X = Value; }
	void SetRightInput(float Value) { CurrentDirectionalInput.Y = Value; }
	void SetUpInput(float Value) { CurrentDirectionalInput.Z = Value; }
	void EnableBoost() { CurrentBoostInput = true; }
	void DisableBoost() { CurrentBoostInput = false; }
	// void SetTurnRightInput(float Val)
	// { CurrentTurnRight = FMath::Clamp(FMath::RoundToInt(Val * 127.0f), -127, 127); }
	// void SetTurnUpInput(float Val)
	// { CurrentTurnUp = FMath::Clamp(FMath::RoundToInt(Val * 127.0f), -127, 127); }
};

