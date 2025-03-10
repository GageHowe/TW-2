#pragma once

#include "CoreMinimal.h"
#include "PhysicsPawn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "SimpleBallPawn.generated.h"

UCLASS()
class BULLETPHYSICSENGINE_API ASimpleBallPawn : public APhysicsPawn
{
	GENERATED_BODY()
	
public:
	ASimpleBallPawn();
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Input handling
	void MoveForward(float Value);
	void MoveRight(float Value);
	void JumpPressed();
	void JumpReleased();
	
	// Process input changes
	void ProcessInput();
	
	// Create input payload
	FBulletPlayerInput CreateInputPayload();

	// Ball properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ball Properties")
	float RollForce = 1500.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ball Properties")
	float JumpForce = 800.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ball Properties")
	float BallRadius = 50.0f;
	
	// Visual components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* BallMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArm;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* Camera;
	
	// Detect if the ball is on the ground
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsOnGround;
	
	// Get direction based on camera orientation
	UFUNCTION(BlueprintPure)
	FVector GetInputDirection() const;
	
	// Apply forces directly for immediate response
	void ApplyLocalForces();
};