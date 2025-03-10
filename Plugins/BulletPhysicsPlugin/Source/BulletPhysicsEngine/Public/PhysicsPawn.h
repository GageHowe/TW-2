#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ControllableInterface.h"
#include "structs.h"
#include "TestActor.h"
#include "PhysicsPawn.generated.h"

UCLASS()
class BULLETPHYSICSENGINE_API APhysicsPawn : public APawn, public IControllableInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	APhysicsPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// Reference to the physics actor that manages the simulation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	ATestActor* PhysicsActor;
	
	// The Bullet physics body ID for this pawn
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	int32 PhysicsBodyID = -1;
	
	// Client ID for input purposes
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	int32 ClientID = -1;
	
	// Last frame we applied an input
	UPROPERTY()
	int32 LastInputFrame = -1;
	
	// Player movement input
	UPROPERTY()
	FVector CurrentMovementInput;
	
	// Player action inputs
	UPROPERTY()
	bool bAbilityPressed;
	
	UPROPERTY()
	bool bCrouchPressed;
	
	UPROPERTY()
	bool bRightClickPressed;
	
	UPROPERTY()
	bool bLeftClickPressed;
	
	// Called from input binding
	UFUNCTION(BlueprintCallable, Category = "Input")
	void MoveForward(float Value);
	
	UFUNCTION(BlueprintCallable, Category = "Input")
	void MoveRight(float Value);
	
	UFUNCTION(BlueprintCallable, Category = "Input")
	void JumpPressed();
	
	UFUNCTION(BlueprintCallable, Category = "Input")
	void JumpReleased();
	
	UFUNCTION(BlueprintCallable, Category = "Input")
	void CrouchPressed();
	
	UFUNCTION(BlueprintCallable, Category = "Input")
	void CrouchReleased();
	
	UFUNCTION(BlueprintCallable, Category = "Input")
	void RightClickPressed();
	
	UFUNCTION(BlueprintCallable, Category = "Input")
	void RightClickReleased();
	
	UFUNCTION(BlueprintCallable, Category = "Input")
	void LeftClickPressed();
	
	UFUNCTION(BlueprintCallable, Category = "Input")
	void LeftClickReleased();
	
	// Process input changes
	UFUNCTION(BlueprintCallable, Category = "Physics Networking")
	void ProcessInput();
	
	// Send inputs to server
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Physics Networking")
	void Server_SendInput(const TArray<FBulletPlayerInput>& Inputs);
	
	// Create input payload from current input state
	UFUNCTION(BlueprintCallable, Category = "Physics Networking")
	FBulletPlayerInput CreateInputPayload();
	
	// IControllableInterface
	virtual void OnPlayerInput_Implementation(const FBulletPlayerInput& Input) override;
	
	// Cache of recent inputs to send redundantly
	UPROPERTY()
	TArray<FBulletPlayerInput> InputCache;
	
	// Maximum redundancy for inputs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	int32 InputRedundancy = REDUNDANT_INPUTS;
	
	// Find the physics actor in the level
	UFUNCTION(BlueprintCallable, Category = "Physics Networking")
	ATestActor* FindPhysicsActor();
	
	// Initialize the rigid body
	UFUNCTION(BlueprintCallable, Category = "Physics Networking")
	void InitializeRigidBody();
	
	// Update transforms from physics 
	UFUNCTION(BlueprintCallable, Category = "Physics Networking")
	void UpdateFromPhysics();
	
	// Override network relevancy for optimization
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;
	
	// Setup replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};