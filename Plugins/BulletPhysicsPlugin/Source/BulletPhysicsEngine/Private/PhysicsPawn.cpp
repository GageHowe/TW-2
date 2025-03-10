#include "PhysicsPawn.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
APhysicsPawn::APhysicsPawn()
{
 	// Set this pawn to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;
	
	// Setup networking
	bReplicates = true;
	SetNetUpdateFrequency(20.0f);
	SetMinNetUpdateFrequency(10.0f);
	
	// Initialize input variables
	CurrentMovementInput = FVector::ZeroVector;
	bAbilityPressed = false;
	bCrouchPressed = false;
	bLeftClickPressed = false;
	bRightClickPressed = false;
}

void APhysicsPawn::BeginPlay()
{
	Super::BeginPlay();
    
	// Find physics actor if not set
	if (!PhysicsActor)
	{
		PhysicsActor = FindPhysicsActor();
        
		if (PhysicsActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("Found PhysicsActor"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to find PhysicsActor!"));
		}
	}
    
	// All clients and the server should initialize physics
	if (PhysicsActor)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, [this]() {
			// Initialize physics after a slight delay
			if (PhysicsBodyID < 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("PhysicsPawn %s initializing rigid body (Client=%d)"), 
					*GetName(), IsLocallyControlled());
				InitializeRigidBody();
			}
		}, 1.0f, false);
	}
}


// Called every frame
void APhysicsPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update based on physics
	UpdateFromPhysics();
	
	// If locally controlled, process input
	if (IsLocallyControlled())
	{
		ProcessInput();
	}
}

// Called to bind functionality to input
void APhysicsPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	// Movement
	PlayerInputComponent->BindAxis("MoveForward", this, &APhysicsPawn::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &APhysicsPawn::MoveRight);
	
	// Actions
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &APhysicsPawn::JumpPressed);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &APhysicsPawn::JumpReleased);
	
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &APhysicsPawn::CrouchPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &APhysicsPawn::CrouchReleased);
	
	PlayerInputComponent->BindAction("LeftClick", IE_Pressed, this, &APhysicsPawn::LeftClickPressed);
	PlayerInputComponent->BindAction("LeftClick", IE_Released, this, &APhysicsPawn::LeftClickReleased);
	
	PlayerInputComponent->BindAction("RightClick", IE_Pressed, this, &APhysicsPawn::RightClickPressed);
	PlayerInputComponent->BindAction("RightClick", IE_Released, this, &APhysicsPawn::RightClickReleased);
}

ATestActor* APhysicsPawn::FindPhysicsActor()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATestActor::StaticClass(), FoundActors);
	
	if (FoundActors.Num() > 0)
	{
		return Cast<ATestActor>(FoundActors[0]);
	}
	
	return nullptr;
}


void APhysicsPawn::InitializeRigidBody()
{
	if (!PhysicsActor)
	{
		UE_LOG(LogTemp, Error, TEXT("InitializeRigidBody: No PhysicsActor found!"));
		return;
	}
    
	if (PhysicsBodyID >= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("InitializeRigidBody: Already initialized with ID %d"), PhysicsBodyID);
		return;
	}
    
	// Client needs to use the ID from the server
	if (!HasAuthority() && !IsLocallyControlled())
	{
		UE_LOG(LogTemp, Warning, TEXT("Waiting for server to initialize physics body..."));
		return;
	}
    
	UE_LOG(LogTemp, Warning, TEXT("Adding rigid body for %s (Auth=%d, Local=%d)"), 
		*GetName(), HasAuthority() ? 1 : 0, IsLocallyControlled() ? 1 : 0);
    
	// Add rigid body to physics simulation
	float Mass = 100.0f;
	float Friction = 0.3f;
	float Restitution = 0.1f;
    
	// Protected by lock in AddRigidBody
	PhysicsActor->AddRigidBody(this, Friction, Restitution, PhysicsBodyID, Mass);
    
	// Set client ID same as body ID
	ClientID = PhysicsBodyID;
    
	// Map client ID to server ID if needed (local client only)
	if (!HasAuthority() && IsLocallyControlled())
	{
		// Add mapping for this client
		PhysicsActor->MapCriticalSection.Lock();
		PhysicsActor->ClientIdToServerId.Add(ClientID, ClientID); // Map to self initially
		PhysicsActor->MapCriticalSection.Unlock();
        
		UE_LOG(LogTemp, Warning, TEXT("Added ID mapping: %d -> %d"), ClientID, ClientID);
	}
    
	UE_LOG(LogTemp, Warning, TEXT("Created physics body with ID %d"), PhysicsBodyID);
}

void APhysicsPawn::UpdateFromPhysics()
{
	if (!PhysicsActor)
	{
		return;
	}
    
	if (PhysicsBodyID < 0)
	{
		return;
	}
    
	// Safety check to make sure PhysicsBodyID is valid
	if (!PhysicsActor->BtRigidBodies.IsValidIndex(PhysicsBodyID))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid PhysicsBodyID: %d (BtRigidBodies size: %d)"), 
			PhysicsBodyID, PhysicsActor->BtRigidBodies.Num());
		return;
	}
    
	FTransform NewTransform;
	FVector Velocity, AngularVelocity, Force;
    
	PhysicsActor->GetPhysicsState(PhysicsBodyID, NewTransform, Velocity, AngularVelocity, Force);
    
	// Update actor transform from physics
	SetActorTransform(NewTransform);
}

void APhysicsPawn::MoveForward(float Value)
{
	CurrentMovementInput.X = FMath::Clamp(Value, -1.0f, 1.0f);
}

void APhysicsPawn::MoveRight(float Value)
{
	CurrentMovementInput.Y = FMath::Clamp(Value, -1.0f, 1.0f);
}

void APhysicsPawn::JumpPressed()
{
	bAbilityPressed = true;
}

void APhysicsPawn::JumpReleased()
{
	bAbilityPressed = false;
}

void APhysicsPawn::CrouchPressed()
{
	bCrouchPressed = true;
}

void APhysicsPawn::CrouchReleased()
{
	bCrouchPressed = false;
}

void APhysicsPawn::LeftClickPressed()
{
	bLeftClickPressed = true;
}

void APhysicsPawn::LeftClickReleased()
{
	bLeftClickPressed = false;
}

void APhysicsPawn::RightClickPressed()
{
	bRightClickPressed = true;
}

void APhysicsPawn::RightClickReleased()
{
	bRightClickPressed = false;
}

FBulletPlayerInput APhysicsPawn::CreateInputPayload()
{
	FBulletPlayerInput Input;
	
	// Set input values
	Input.MovementInput = CurrentMovementInput;
	Input.Ability = bAbilityPressed;
	Input.Crouch = bCrouchPressed;
	Input.LeftClick = bLeftClickPressed;
	Input.RightClick = bRightClickPressed;
	
	// Set ID and frame number
	Input.ObjectID = ClientID;
	
	if (PhysicsActor)
	{
		Input.FrameNumber = PhysicsActor->CurrentFrameNumber;
	}
	
	return Input;
}

void APhysicsPawn::ProcessInput()
{
	// Skip if no physics actor or not initialized
	if (!PhysicsActor || PhysicsBodyID < 0)
	{
		return;
	}
	
	// Only process input at the physics frame rate
	int32 CurrentFrame = PhysicsActor->CurrentFrameNumber;
	if (LastInputFrame == CurrentFrame)
	{
		return;
	}
	
	LastInputFrame = CurrentFrame;
	
	// Create input payload
	FBulletPlayerInput Input = CreateInputPayload();
	
	// Add to local cache (for redundant sending)
	InputCache.Add(Input);
	
	// Limit cache size
	if (InputCache.Num() > InputRedundancy)
	{
		InputCache.RemoveAt(0, InputCache.Num() - InputRedundancy);
	}
	
	// Apply locally for prediction
	PhysicsActor->ProcessClientInput(Input);
	
	// Send to server
	Server_SendInput(InputCache);
}

// Add this to PhysicsPawn.cpp - improved Server_SendInput_Implementation
void APhysicsPawn::Server_SendInput_Implementation(const TArray<FBulletPlayerInput>& Inputs)
{
	if (!PhysicsActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_SendInput: No PhysicsActor found"));
		return;
	}
    
	// Log for debugging
	UE_LOG(LogTemp, Warning, TEXT("SERVER: Received %d inputs from client %d"), Inputs.Num(), ClientID);
    
	// Process all inputs from client
	for (const FBulletPlayerInput& Input : Inputs)
	{
		// Create a modified input with the correct client ID
		FBulletPlayerInput ModifiedInput = Input;
		ModifiedInput.ObjectID = ClientID; // Use the server's ID for this client
        
		// Add to server's input buffer
		PhysicsActor->EnqueueInput(ModifiedInput, ClientID);
        
		// Apply input directly for immediate response
		PhysicsActor->ProcessClientInput(ModifiedInput);
	}
}

void APhysicsPawn::OnPlayerInput_Implementation(const FBulletPlayerInput& Input)
{
	// Custom handling of input for this specific pawn type
	// This is called by the physics actor after the input is applied
	
	// Apply additional game-specific effects here based on input
	// For example: play sounds, effects, etc.
}

bool APhysicsPawn::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	// Always relevant for owner
	if (GetInstigator() == ViewTarget)
	{
		return true;
	}
	
	// Otherwise use standard relevancy check
	return Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
}

void APhysicsPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// Replicate physics info
	DOREPLIFETIME(APhysicsPawn, PhysicsBodyID);
	DOREPLIFETIME(APhysicsPawn, ClientID);
}