//Robert Sebenda, Manuel Cebreros
//Copyright 2016 

#pragma once

#include "GameFramework/Actor.h"
#include "SocketData.h"
#include "StaticLibrary.h"
#include "WeaponBase.generated.h"


UCLASS()
class BLOODANDSTEEL_API AWeaponBase : public AActor
{
	GENERATED_BODY()


	float cooldownTimer;

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float collisionCooldown;

	UPROPERTY(Replicated,EditAnywhere, BlueprintReadOnly, Category = "Knight")
	class AKnightBase* ownerKnight;
	
	// Sets default values for this actor's properties
	AWeaponBase(const FObjectInitializer& ObjectInitializer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsOnCooldown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FeedBack")
		TArray<UParticleSystem*> sparks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FeedBack")
		TArray<USoundCue*> SwordHitSounds;


	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float velocity;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float tipRecoilMultiplier;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float baseRecoilMultiplier;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float centerRecoilMultiplier;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float weaponRecoilMultiplier;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float WeaponStaminaModifier;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float WeaponSprintStaminaDrainModifier;
	//weapon "weight" will affect the movespeed of the knight 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float WeaponSprintModifier;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float WeaponWalkModifer;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float weaponStaggerModifer;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float intertia;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float speed;

	//damage ranges




	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
		FVector lastPosition;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 currentSocketCast;

	void UpdateVelocity(AActor swordPart);
	
	void TestCollision(float partVelocity, AActor* otherActor);

	void PlaySparkHit(UWorld* world, FVector spawnPoint);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void GetAllSockets();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TArray<USocketData*> sockets;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
		void UpdateSocketData();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void OnCollisionDetected(FHitResult const &hitResult);

	UFUNCTION(Unreliable,NetMulticast, WithValidation, Category = "Combat")
	void NET_OnCollisionResolved(ECollisionType collisionType,FVector impactPoint, FName bone);
	void NET_OnCollisionResolved_Implementation(ECollisionType collisionType, FVector impactPoint, FName bone);
	bool NET_OnCollisionResolved_Validate(ECollisionType collisionType, FVector impactPoint, FName bone);

	UFUNCTION()
	void OnKnightDestruction();

	//weapon modifiers
	//to be init in blueprint for each weapon

	//base Daamge for weapons
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifiers")
		float weaponBaseDamage;

	//speed modifier determines how fast the weapon swings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifiers")
		float weaponSpeedBase;

	//Inertia determines how much follow through on weapon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifiers")
		float weaponInertia;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifiers")
		float weaponRecoilModifier;

	//called when hit by an object
	UFUNCTION(BlueprintCallable, Category = "Combat")
		void OnHit(FVector hitLocation, float forceOfHit);

	//FeedBack Effects

	UFUNCTION(Reliable, NetMulticast,WithValidation, BlueprintCallable, Category = "FeedBack")
	void NET_Sword_Feedback(FVector hitLocation, float forceOfHit);
	void NET_Sword_Feedback_Implementation(FVector hitLocation, float forceOfHit);
	bool NET_Sword_Feedback_Validate(FVector hitLocation, float forceOfHit);

	UFUNCTION(BlueprintImplementableEvent)
		void FeedBackSwordHit(FVector hitLocation, EDamageType damageType);

	
	FName GetSwordType();


};
