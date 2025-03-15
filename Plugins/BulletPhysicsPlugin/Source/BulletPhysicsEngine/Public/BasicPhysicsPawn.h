#pragma once

#include "CoreMinimal.h"
#include "TestActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "helpers.h"
#include "bthelper.h"
#include "Net/UnrealNetwork.h"
#include "TWRingBuffer.h"
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
	
	FVector CurrentDirectionalInput = FVector(0, 0, 0);
	bool CurrentPrimaryInput = false;
	bool CurrentSecondaryInput = false;
	bool CurrentBoostInput = false;
	int8 CurrentTurnRight = 0;
	int8 CurrentTurnUp = 0;
	
	btRigidBody* MyRigidBody = nullptr;
	
	UPROPERTY(EditAnywhere)
	ATestActor* BulletWorld = nullptr;
	
	// this is marked false when the pawn should not send or receive input
	// i.e. an inactive vehicle or dead player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	bool IsPossessed = true;

	UFUNCTION()
	void ApplyInputs(const FTWPlayerInput& input) const;
	void EnableDebug();
	
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
	void SetTurnRightInput(float Val)
	{ CurrentTurnRight = FMath::Clamp(FMath::RoundToInt(Val * 127.0f), -127, 127); }
	void SetTurnUpInput(float Val)
	{ CurrentTurnUp = FMath::Clamp(FMath::RoundToInt(Val * 127.0f), -127, 127); }

	void Interpolate()
	{
		if (auto x = BulletWorld->InterpolationError)
		{
			// x.
		}
	}
};

