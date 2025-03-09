#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TestActor.h"
#include "PhysicsGameMode.generated.h"

/**
 * Game mode for physics-based networked gameplay
 */
UCLASS()
class BULLETPHYSICSENGINE_API APhysicsGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	APhysicsGameMode();
	
	virtual void BeginPlay() override;
	
	virtual void PostLogin(APlayerController* NewPlayer) override;
	
	// Reference to the physics manager
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	ATestActor* PhysicsActor;
	
	// Get the physics actor (creates one if not found)
	UFUNCTION(BlueprintCallable, Category = "Physics Networking")
	ATestActor* GetPhysicsActor();
	
	// Create physics objects for the level
	UFUNCTION(BlueprintCallable, Category = "Physics Networking")
	void SetupLevelPhysics();
};