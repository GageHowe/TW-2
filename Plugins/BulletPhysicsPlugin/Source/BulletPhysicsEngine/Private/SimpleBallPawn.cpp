#include "SimpleBallPawn.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

ASimpleBallPawn::ASimpleBallPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // Set up collision component
    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
    CollisionComponent->SetSphereRadius(BallRadius);
    CollisionComponent->SetCollisionProfileName(TEXT("Pawn"));
    SetRootComponent(CollisionComponent);
    
    // Set up ball mesh
    BallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BallMesh"));
    BallMesh->SetupAttachment(RootComponent);
    BallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    
    // Load a default sphere mesh
    static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultSphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (DefaultSphereMesh.Succeeded())
    {
        BallMesh->SetStaticMesh(DefaultSphereMesh.Object);
    }
    
    // Set up spring arm
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->TargetArmLength = 300.0f;
    SpringArm->bUsePawnControlRotation = true;
    
    // Set up camera
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);
    Camera->bUsePawnControlRotation = false;
    
    // Set up movement replication
    bReplicates = true;
}

void ASimpleBallPawn::BeginPlay()
{
    Super::BeginPlay();
    
    // Scale the collision and mesh properly
    CollisionComponent->SetSphereRadius(BallRadius);
    
    // Scale the mesh properly (assuming default sphere is 100 units diameter)
    const float MeshScale = (BallRadius * 2.0f) / 100.0f;
    BallMesh->SetRelativeScale3D(FVector(MeshScale, MeshScale, MeshScale));
}

void ASimpleBallPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Check if on ground
    FHitResult Hit;
    FVector Start = GetActorLocation();
    FVector End = Start - FVector(0, 0, BallRadius + 5.0f);
    
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    
    bIsOnGround = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
    
    // If we're locally controlled, send inputs to server
    if (IsLocallyControlled())
    {
        // Check for any input changes
        static FVector LastInput = FVector::ZeroVector;
        static bool LastJump = false;
        
        bool InputChanged = !CurrentMovementInput.Equals(LastInput, 0.01f) || bAbilityPressed != LastJump;
        
        if (InputChanged)
        {
            LastInput = CurrentMovementInput;
            LastJump = bAbilityPressed;
            
            // Send input at least once per frame
            ProcessInput();
        }
        
        // Always apply forces directly on locally controlled pawns
        if (PhysicsActor && PhysicsBodyID >= 0 && 
            PhysicsActor->BtRigidBodies.IsValidIndex(PhysicsBodyID))
        {
            // Apply immediate forces
            ApplyLocalForces();
            
            UE_LOG(LogTemp, VeryVerbose, TEXT("Applied local forces: %s"),
                  *GetInputDirection().ToString());
        }
    }
}

void ASimpleBallPawn::ApplyLocalForces()
{
    // Safety check
    if (!PhysicsActor || PhysicsBodyID < 0 || 
        !PhysicsActor->BtRigidBodies.IsValidIndex(PhysicsBodyID))
    {
        return;
    }
    
    // Apply movement forces
    if (!CurrentMovementInput.IsNearlyZero())
    {
        // Get direction based on camera orientation
        FVector InputDir = GetInputDirection() * RollForce;
        
        // Apply force directly
        PhysicsActor->AddCentralForce(PhysicsBodyID, InputDir);
    }
    
    // Apply jump force
    if (bAbilityPressed && bIsOnGround)
    {
        PhysicsActor->AddImpulse(PhysicsBodyID, FVector(0, 0, JumpForce), GetActorLocation());
        bAbilityPressed = false; // Consume jump
    }
}

void ASimpleBallPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    
    // Bind input functions
    PlayerInputComponent->BindAxis("MoveForward", this, &ASimpleBallPawn::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &ASimpleBallPawn::MoveRight);
    
    PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASimpleBallPawn::JumpPressed);
    PlayerInputComponent->BindAction("Jump", IE_Released, this, &ASimpleBallPawn::JumpReleased);
}

void ASimpleBallPawn::MoveForward(float Value)
{
    CurrentMovementInput.X = Value;
}

void ASimpleBallPawn::MoveRight(float Value)
{
    CurrentMovementInput.Y = Value;
}

void ASimpleBallPawn::JumpPressed()
{
    bAbilityPressed = true;
}

void ASimpleBallPawn::JumpReleased()
{
    bAbilityPressed = false;
}

FVector ASimpleBallPawn::GetInputDirection() const
{
    if (!Controller)
    {
        return FVector::ZeroVector;
    }
    
    // Get the control rotation and extract the yaw
    const FRotator Rotation = Controller->GetControlRotation();
    const FRotator YawRotation(0, Rotation.Yaw, 0);
    
    // Forward and right vectors
    const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
    
    // Combine with input values
    return (ForwardDir * CurrentMovementInput.X + RightDir * CurrentMovementInput.Y).GetSafeNormal();
}

void ASimpleBallPawn::ProcessInput()
{
    if (!PhysicsActor || PhysicsBodyID < 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessInput: Not ready (PhysicsActor=%p, PhysicsBodyID=%d)"), 
               PhysicsActor, PhysicsBodyID);
        return;
    }
    
    // Create and send input to server
    FBulletPlayerInput Input = CreateInputPayload();
    
    // Debug output for input sending
    UE_LOG(LogTemp, Warning, TEXT("CLIENT %d: Creating input with movement (%f, %f), ability=%d"),
           PhysicsBodyID, Input.MovementInput.X, Input.MovementInput.Y, Input.Ability ? 1 : 0);
    
    // Add to input cache
    InputCache.Add(Input);
    if (InputCache.Num() > InputRedundancy)
    {
        InputCache.RemoveAt(0, InputCache.Num() - InputRedundancy);
    }
    
    // Always send current input to server
    if (IsLocallyControlled())
    {
        TArray<FBulletPlayerInput> SingleInput;
        SingleInput.Add(Input);
        
        UE_LOG(LogTemp, Warning, TEXT("CLIENT: Sending input to server"));
        Server_SendInput(SingleInput);
    }
}

FBulletPlayerInput ASimpleBallPawn::CreateInputPayload()
{
    FBulletPlayerInput Input;
    
    // Set raw input values without multiplying by force (apply force on server)
    Input.MovementInput = GetInputDirection();
    Input.Ability = bAbilityPressed && bIsOnGround;
    Input.Crouch = bCrouchPressed;
    Input.LeftClick = bLeftClickPressed;
    Input.RightClick = bRightClickPressed;
    
    // Set ID using the client ID, not physics body ID
    Input.ObjectID = ClientID;
    
    if (PhysicsActor)
    {
        Input.FrameNumber = PhysicsActor->CurrentFrameNumber;
    }
    
    return Input;
}
