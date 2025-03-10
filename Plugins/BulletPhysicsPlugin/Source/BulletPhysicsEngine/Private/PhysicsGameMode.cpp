#include "PhysicsGameMode.h"
#include "SimpleBallPawn.h"
#include "Kismet/GameplayStatics.h"

APhysicsGameMode::APhysicsGameMode()
{
    // Set default pawn class to our ball
    DefaultPawnClass = ASimpleBallPawn::StaticClass();
}

void APhysicsGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    // Get or create physics actor
    PhysicsActor = GetPhysicsActor();
    
    // Setup gravity for a rolling ball game
    if (PhysicsActor && PhysicsActor->BtWorld)
    {
        PhysicsActor->BtWorld->setGravity(btVector3(0, 0, -980.0f));
    }
    
    // Setup level physics
    SetupLevelPhysics();
}

void APhysicsGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    
    // Wait a bit to ensure pawn is spawned and replicated
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, [this, NewPlayer]() {
        APhysicsPawn* PlayerPawn = Cast<APhysicsPawn>(NewPlayer->GetPawn());
        if (PlayerPawn && PhysicsActor)
        {
            UE_LOG(LogTemp, Warning, TEXT("Initializing physics for player %s"), 
                  *NewPlayer->GetName());
            
            // Set the physics actor reference
            PlayerPawn->PhysicsActor = PhysicsActor;
            
            // Server initializes rigid bodies for all players
            if (NewPlayer->HasAuthority() && PlayerPawn->PhysicsBodyID < 0)
            {
                // Init and get ID
                PlayerPawn->InitializeRigidBody();
                
                UE_LOG(LogTemp, Warning, TEXT("Server initialized physics body ID %d for player %s"), 
                      PlayerPawn->PhysicsBodyID, *NewPlayer->GetName());
                
                // Setup ID mapping
                PhysicsActor->MapCriticalSection.Lock();
                PhysicsActor->ServerIdToClientId.Add(PlayerPawn->PhysicsBodyID, PlayerPawn->PhysicsBodyID);
                PhysicsActor->MapCriticalSection.Unlock();
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("PostLogin: PlayerPawn or PhysicsActor is null"));
        }
    }, 1.5f, false);
}

ATestActor* APhysicsGameMode::GetPhysicsActor()
{
    // Find existing physics actor
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATestActor::StaticClass(), FoundActors);
    
    if (FoundActors.Num() > 0)
    {
        return Cast<ATestActor>(FoundActors[0]);
    }
    
    // Create new one if needed
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    return GetWorld()->SpawnActor<ATestActor>(ATestActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
}

void APhysicsGameMode::SetupLevelPhysics()
{
    if (!PhysicsActor)
    {
        return;
    }
    
    // Add static geometry from level
    TArray<AActor*> StaticActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("StaticPhysics"), StaticActors);
    
    if (StaticActors.Num() > 0)
    {
        PhysicsActor->SetupStaticGeometryPhysics(StaticActors, 0.8f, 0.3f);
    }
    
    // Add default ground plane if needed
    if (StaticActors.Num() == 0)
    {
        btStaticPlaneShape* GroundShape = new btStaticPlaneShape(btVector3(0, 0, 1), 0);
        PhysicsActor->AddStaticCollision(GroundShape, FTransform::Identity, 0.8f, 0.3f, nullptr);
    }
}