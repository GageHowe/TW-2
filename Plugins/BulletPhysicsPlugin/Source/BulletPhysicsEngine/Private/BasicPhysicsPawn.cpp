#include "BasicPhysicsPawn.h"

#include "TestActor.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "TWPlayerController.h"

ABasicPhysicsPawn::ABasicPhysicsPawn() : StateHistoryBuffer(128)
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(false);
	
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlayerMesh"));
	RootComponent = StaticMesh;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	Camera->SetupAttachment(StaticMesh);
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

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
}

void ABasicPhysicsPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (IsLocallyControlled())
	{
		FBulletPlayerInput input = FBulletPlayerInput();
		input.MovementInput = DirectionalInput;
		ApplyInputs(input);
		SendInputsToServer(this, input);
	}

	// world timer stuff
	// double x = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	// GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, 
	// FString::Printf(TEXT("time: %f"), x));
}

// right now, this just accelerates the pawn, but that's fine for now
void ABasicPhysicsPawn::ApplyInputs(const FBulletPlayerInput& input) const
{
	GEngine->AddOnScreenDebugMessage(3, 5.0f, FColor::Green, TEXT("Applying inputs"));
	btVector3 forceVector = BulletHelpers::ToBtDir(input.MovementInput, true) * 10000.0f;
	if (!MyRigidBody) { GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("WARNING RigidBody ptr is null 2")); return; }
	MyRigidBody->applyForce(forceVector, btVector3(0, 0, 0));
}

void ABasicPhysicsPawn::ServerTestSimple_Implementation()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Simple RPC worked!"));
}

void ABasicPhysicsPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	IsPossessed = true;
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
	InputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &ABasicPhysicsPawn::EnableDebug);
}

void ABasicPhysicsPawn::EnableDebug()
{
	if (GetController())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, 
			FString::Printf(TEXT("Controller: %s"), *GetController()->GetName()));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("No Controller!"));
	}
	
	if (auto pc = Cast<ATWPlayerController>(GetController()))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("Correct Player Controller Found"));
		pc->SyncTimeWithServer();
	}
}

void ABasicPhysicsPawn::SendInputsToServer_Implementation(AActor* actor, FBulletPlayerInput input)
{
	BulletWorld->SendInputToServer(this, input);
}
