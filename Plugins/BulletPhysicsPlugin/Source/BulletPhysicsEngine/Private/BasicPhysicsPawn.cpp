#include "BasicPhysicsPawn.h"

#include "TestActor.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

ABasicPhysicsPawn::ABasicPhysicsPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlayerMesh"));
	RootComponent = StaticMesh;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	Camera->SetupAttachment(StaticMesh);
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SetReplicatingMovement(false);
	SetReplicates(true);
}

void ABasicPhysicsPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABasicPhysicsPawn, testdebug);
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

	if (GetLocalRole() == ROLE_Authority)
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Green, TEXT("I am the SERVER"));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("I am a CLIENT"));
	}
}

void ABasicPhysicsPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocallyControlled())
	{
		// locally simulate inputs
		FBulletPlayerInput input = FBulletPlayerInput();
		input.MovementInput = DirectionalInput;
		ApplyInputs(input);

		// send inputs to server
		// FNetworkGUID id = BulletWorld->GetNetGUIDFromActor(this);
		// BulletWorld->SR_SendInputsByID(id.ObjectId, input);
	} else if (HasAuthority())
	{
		// BulletWorld->SendInputsToServer
		// printf("slkdf");
	}

	// debug stuff
	if (testdebug)
	{
		if (IsLocallyControlled())
		{
			auto id = BulletWorld->GetNetGUIDFromActor(this).ObjectId;
			GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString::Printf(TEXT("Client: ID: %llu"), id));
			SR_PrintID();
		}


	}
	if (HasAuthority() && BulletWorld)
	{
		auto id = BulletWorld->GetNetGUIDFromActor(this).ObjectId;
		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Blue, FString::Printf(TEXT("Server: ID: %llu"), id));
	}
	else if (HasAuthority())
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Red, TEXT("BulletWorld is null on server"));
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

void ABasicPhysicsPawn::PossessedBy(AController* NewController)
{
	IsPossessed = true;
}

void ABasicPhysicsPawn::UnPossessed()
{
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
	testdebug = true;
}

void ABasicPhysicsPawn::SR_PrintID_Implementation()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("SR_PrintID - Executing on SERVER"));
    
	// Check if BulletWorld is valid
	if (BulletWorld)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("BulletWorld is valid on server"));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("BulletWorld is NULL on server"));
		return;
	}
    
	// Try to get the NetGUID, with more debug info
	FNetworkGUID netGUID;
	try
	{
		netGUID = BulletWorld->GetNetGUIDFromActor(this);
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Got NetGUID successfully"));
	}
	catch(...)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Exception when calling GetNetGUIDFromActor"));
		return;
	}
    
	// If we got here, try to get the ObjectId
	try
	{
		uint64 id = netGUID.ObjectId;
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, FString::Printf(TEXT("Server: ID: %llu"), id));
	}
	catch(...)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Exception when accessing ObjectId"));
	}
}