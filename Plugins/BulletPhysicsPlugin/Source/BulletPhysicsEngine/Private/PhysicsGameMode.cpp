#include "PhysicsGameMode.h"
#include "PhysicsPawn.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"

APhysicsGameMode::APhysicsGameMode()
{
    // Set default pawn class
    DefaultPawnClass = APhysicsPawn::StaticClass();
}

void APhysicsGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    // Get or create physics actor
    PhysicsActor = GetPhysicsActor();
    
    // Setup level physics objects
    SetupLevelPhysics();
}

void APhysicsGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    
    // Ensure the physics actor is set on the player's pawn
    APhysicsPawn* PlayerPawn = Cast<APhysicsPawn>(NewPlayer->GetPawn());
    if (PlayerPawn && PhysicsActor)
    {
        PlayerPawn->PhysicsActor = PhysicsActor;
        
        // Initialize physics for new player
        if (PlayerPawn->PhysicsBodyID < 0)
        {
            PlayerPawn->InitializeRigidBody();
        }
    }
}

ATestActor* APhysicsGameMode::GetPhysicsActor()
{
    // Try to find existing physics actor
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATestActor::StaticClass(), FoundActors);
    
    if (FoundActors.Num() > 0)
    {
        return Cast<ATestActor>(FoundActors[0]);
    }
    
    // Create a new physics actor if none exists
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    ATestActor* NewPhysicsActor = GetWorld()->SpawnActor<ATestActor>(ATestActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    
    if (NewPhysicsActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("Created new physics actor"));
    }
    
    return NewPhysicsActor;
}

void APhysicsGameMode::SetupLevelPhysics()
{
    if (!PhysicsActor)
    {
        UE_LOG(LogTemp, Error, TEXT("No physics actor available to setup level physics!"));
        return;
    }
    
    // Add static geometry from the level
    TArray<AActor*> StaticActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("StaticPhysics"), StaticActors);
    
    if (StaticActors.Num() > 0)
    {
        PhysicsActor->SetupStaticGeometryPhysics(StaticActors, 0.8f, 0.3f);
        UE_LOG(LogTemp, Log, TEXT("Added %d static physics objects"), StaticActors.Num());
    }
    
    // Add the floor as a static plane if needed
    // This is an example of how to create a static ground plane
    /*
    btStaticPlaneShape* GroundShape = new btStaticPlaneShape(btVector3(0, 0, 1), 0); // Facing up, at z=0
    btTransform GroundTransform;
    GroundTransform.setIdentity();
    
    PhysicsActor->AddStaticCollision(GroundShape, FTransform::Identity, 0.8f, 0.1f, nullptr);
    */
}