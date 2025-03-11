#pragma once

#include "CoreMinimal.h"
#include "TestActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
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
	
	FVector DirectionalInput = FVector(0, 0, 0);
	
	// this is marked false when the pawn should not send or receive input
	// i.e. an inactive vehicle or dead player
	bool IsActive = true; // fix this to only be true when possessed
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
