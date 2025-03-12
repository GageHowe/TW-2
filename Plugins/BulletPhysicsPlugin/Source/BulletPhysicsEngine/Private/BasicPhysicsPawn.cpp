#include "BasicPhysicsPawn.h"

#include "TestActor.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

ABasicPhysicsPawn::ABasicPhysicsPawn()
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
	// DOREPLIFETIME(ABasicPhysicsPawn, testdebug);
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
	btRigidBody* rb = world->AddRigidBodyAndReturn(this, 0.2, 0.2, 1);
	if (!rb) { GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("WARNING RigidBody ptr is null")); }
	MyRigidBody = rb;

	// if (GetLocalRole() == ROLE_Authority)
	// {
	// 	GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Green, TEXT("I am the SERVER"));
	// }
	// else
	// {
	// 	GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("I am a CLIENT"));
	// }

	bReplicates = true;
	SetOwner(GetController());
	
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, 
	FString::Printf(TEXT("Network Role: %d"), GetLocalRole()));
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
}

// right now, this just accelerates the pawn, but that's fine for now
void ABasicPhysicsPawn::ApplyInputs(const FBulletPlayerInput& input) const
{
	// GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Applying inputs"));
	btVector3 forceVector = BulletHelpers::ToBtDir(input.MovementInput, true) * 10000.0f;
	if (!MyRigidBody) { GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("WARNING RigidBody ptr is null 2")); return; }
	MyRigidBody->applyForce(forceVector, btVector3(0, 0, 0));
}

void ABasicPhysicsPawn::ServerTest_Implementation()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("server rpc in pawn worked!"));
    
	IsPossessed = false;
	if (BulletWorld)
	{
		BulletWorld->test2();
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("BulletWorld is null!"));
	}
}

// In your implementation file (.cpp)
bool ABasicPhysicsPawn::ServerTest_Validate()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("RPC Validation"));
	return true;
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
    
	ServerTestSimple();
	ServerTest();
}

void ABasicPhysicsPawn::SendInputsToServer_Implementation(AActor* actor, FBulletPlayerInput input)
{
	BulletWorld->SendInputToServer(this, input);
}
