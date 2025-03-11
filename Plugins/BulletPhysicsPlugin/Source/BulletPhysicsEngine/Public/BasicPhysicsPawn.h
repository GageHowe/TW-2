#pragma once

#include "CoreMinimal.h"
#include "TestActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "BasicPhysicsPawn.generated.h"

UCLASS()
class BULLETPHYSICSENGINE_API ABasicPhysicsPawn : public APawn
{
	GENERATED_BODY()
public:
	ABasicPhysicsPawn();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	virtual void BeginPlay() override;
	FVector DirectionalInput = FVector(0, 0, 0);
private:
	UPROPERTY(EditDefaultsOnly)
	UStaticMeshComponent* StaticMesh;

	UPROPERTY(EditDefaultsOnly)
	UCameraComponent* Camera;
protected:
	UPROPERTY(EditDefaultsOnly)
	ATestActor* world;
	
	void SetForwardInput(float Value)
	{
		// confirmed to work
		// if (GEngine) { GEngine->AddOnScreenDebugMessage(-1,5,FColor::Red,"Forward Input"); }
	}
	void SetRightInput(float Value)
	{
		
	}
	void SetUpInput(float Value)
	{
		
	}
};
