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
	
	// this is marked false when the pawn should not send or receive input
	// i.e. an inactive vehicle or dead player
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsPossessed = true; // fix this to only be true when possessed

	void ApplyInputs(const FBulletPlayerInput& input) const;
	// void EnableDebug();

	UFUNCTION(Server, Reliable)
	void ServerTest();
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

