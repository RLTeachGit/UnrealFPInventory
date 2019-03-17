// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupActor.generated.h"


UCLASS()
class UNREALFPINVENTORY_API APickupActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APickupActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


public: //Set up up default components
	UPROPERTY()
	class USceneComponent* PickupRoot; //Root to be parent of PickupMesh

	UPROPERTY(EditAnywhere, Category = Mesh)
	class USceneComponent* WorldDepiction;

	UPROPERTY(EditAnywhere, Category = Mesh)
	class USceneComponent* OnPlayerDepiction;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnPickedup(AUnrealFPInventoryCharacter* OwningActor); //Send who picked us up, implemented in BP

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnPickupTick(float DeltaTime, float TimeAlive); //DeltaTime and Time Alive so far

	UFUNCTION(BlueprintGetter)
	float	TimeAlive(); //Get Time Alive

	UFUNCTION(BlueprintGetter)
	bool	IsPickedUp(); //Get Pickupflag

private:

	UFUNCTION()	//As we are dynamically adding this we need it to be a UFUNCTION()
	void OnOverlap(AActor * MyActor, AActor * OtherActor);

	bool	isPickedUp; //Flag to show if we are picked up
	float	CurrentTime; //Keep total time since start
};
