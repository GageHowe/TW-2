#include "BasicPhysicsPawn.h"

#include "TestActor.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "TWPlayerController.h"
#include "GameFramework/SpringArmComponent.h"

ABasicPhysicsPawn::ABasicPhysicsPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bAsyncPhysicsTickEnabled = true;
	bReplicates = true;
	SetReplicatingMovement(false);
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlayerMesh"));
	RootComponent = StaticMesh;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	
	// SetReplicates(true);
}

void ABasicPhysicsPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABasicPhysicsPawn, IsPossessed);
}

void ABasicPhysicsPawn::BeginPlay()
{
	Super::BeginPlay();
	
	TArray<AActor*> worlds;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATestActor::StaticClass(), worlds);
	world = Cast<ATestActor>(worlds[0]); // this will crash if no world is present
										// if you ain't crashed, the reference is valid
	BulletWorld = world;
	MyRigidBody = world->AddRigidBodyAndReturn(this, 0.2, 0.2, 1);
	if (!MyRigidBody) { GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("WARNING RigidBody ptr is null")); }

	if (IsLocallyControlled())
	{
		BulletWorld->LocalPawn = this;
	}

	BulletWorld->ActorToBody.Add(this, MyRigidBody);
	BulletWorld->BodyToActor.Add(MyRigidBody, this);
}

void ABasicPhysicsPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// physics and movement handling moved to async physics tick
}

void ABasicPhysicsPawn::AsyncPhysicsTickActor(float DeltaTime, float SimTime)
{
	Super::AsyncPhysicsTickActor(DeltaTime, SimTime);

	// send inputs to the server
	if (IsLocallyControlled())
	{
		FTWPlayerInput input = FTWPlayerInput();
		input.MovementInput = CurrentDirectionalInput;
		input.TurnRight = CurrentTurnRight;
		input.TurnUp = CurrentTurnUp;
		input.RollRight = CurrentRollRight;
		input.BoostInput = CurrentBoostInput;
		// input.RotationInput = GetControlRotation(); // depricated
		BulletWorld->LocalInputBuffer.Push(input);
		ApplyInputs(input);
		if (!HasAuthority()) {SendInputsToServer(this, input);}
	}
}

// override this function when creating children
void ABasicPhysicsPawn::ApplyInputs(const FTWPlayerInput& input) {}

void ABasicPhysicsPawn::ServerTestSimple_Implementation()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Simple RPC worked!"));
}

void ABasicPhysicsPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	IsPossessed = true;
	if (BulletWorld && IsLocallyControlled())
	{
		BulletWorld->LocalPawn = this;
	}
}

void ABasicPhysicsPawn::UnPossessed()
{
	Super::UnPossessed();
	IsPossessed = false;
}

void ABasicPhysicsPawn::SetupPlayerInputComponent(class UInputComponent* ThisInputComponent)
{
	Super::SetupPlayerInputComponent(ThisInputComponent);
	InputComponent->BindAxis(TEXT("MoveForward"), this, &ABasicPhysicsPawn::SetForwardInput);
	InputComponent->BindAxis(TEXT("MoveRight"), this, &ABasicPhysicsPawn::SetRightInput);
	InputComponent->BindAxis(TEXT("MoveUp"), this, &ABasicPhysicsPawn::SetUpInput);

	InputComponent->BindAction(TEXT("Boost"), IE_Pressed, this, &ABasicPhysicsPawn::EnableBoost);
	InputComponent->BindAction(TEXT("Boost"), IE_Released, this, &ABasicPhysicsPawn::DisableBoost);

	// no longer are we directly controlling the pawn with the input
	InputComponent->BindAxis(TEXT("LookRight"), this, &ABasicPhysicsPawn::SetTurnRightInput);
	InputComponent->BindAxis(TEXT("LookUp"), this, &ABasicPhysicsPawn::SetTurnUpInput);
	InputComponent->BindAxis(TEXT("Roll"), this, &ABasicPhysicsPawn::SetRollRightInput);
	//
	
	// // camera free look
	// InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	// InputComponent->BindAxis("LookRight", this, &APawn::AddControllerYawInput);
}

void ABasicPhysicsPawn::SendInputsToServer_Implementation(AActor* actor, FTWPlayerInput input)
{
	BulletWorld->SendInputToServer(this, input);
}
