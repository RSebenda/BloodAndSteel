// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Character.h"
#include "BaS_ArmorBaseComponent.h"
#include "WeaponBase.h"
#include "StaticLibrary.h"
#include "KnightBase.generated.h"




UCLASS(config = Game)
class AKnightBase : public ACharacter
{
	GENERATED_BODY()

public:
	AKnightBase();

	//constants 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Knight")
	float BASE_HEALTH = 100.0f;
	//const for weapon base pose
	float const weaponPitchBase = 0;
	float const MAX_CAMERA_ROTATION = 25.0f;
	// value for interp for weaponPitch and yaw 
	//when exiting combat
	float const MOMENTUMSLOWDOWNSPEED = 2.0f;
	float const INTERPOLATEVALUEEXIT = 2.0f;
	// value for interp when entering combat //higher value moves faster
	float const INTERPOLATEVALUEENTER = 6.0f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
	AWeaponBase* ownWeapon;

	UFUNCTION(BlueprintCallable, Category = "KnightState")
	void SpawnOwnSword();
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float BaseLookUpRate;

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Session Search Completed"))
		void OnLockTargetStart();

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Session Search Completed"))
		void OnLockOnTargetStop();
	
	//DEATH stuff

	//how long a player remains dead before trying to respawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Tunning")
	float deathTime;

	//handle for when a player requests to respawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Tunning")
	FTimerHandle respawnHandle;

	UFUNCTION(BlueprintCallable, Category = "Respawn")
		void Respawn();

	//respawn for player who gets kill
	UFUNCTION(BlueprintCallable, Category = "Respawn")
		void WinnerRespawn();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Tunning")
	float respawnTime;

	//value where if damage greater than, knight will be dismembered
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Tunning")
	float dismembermentThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Tunning")
		float lowHealthThreshold;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Knight State")
	float Health;
	
	//inits player on spawn
	void InitializeState();

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Knight State")
	bool bIsAlive;

	/** First person camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	class UCameraComponent* FirstPersonCameraComponent;

	FORCEINLINE class UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	//Blood particles
	UPROPERTY(EditAnywhere, Category = "Particles")
	class UParticleSystemComponent* bloodPSC;

	UPROPERTY(EditAnywhere,BlueprintReadOnly,Replicated, Category = "Knight Sounds")
	bool IsHelmetOn;

	UPROPERTY(EditAnywhere, Category = "Knight Sounds")
	class UAudioComponent* voiceComponent;

	//camera shake when player gets hit, set in BP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Handling")
	TSubclassOf<class UCameraShake> cameraShakeTemplate;

	FRotator cameraStartRotation;

	//current target lockon
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category ="Knight state")
	AKnightBase* currentTarget;

	UPROPERTY(VisibleAnywhere, Category = "Knight state")
		AKnightBase* combatWith;

	UPROPERTY(EditAnywhere)
	USphereComponent* lockOnTriggerArea;

	UPROPERTY(EditAnywhere)
		FVector camOffset = FVector(0, 0, 64.f);

	// Sword Movement Section
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Weapon Position")
		float cppWeaponPitch;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Weapon Position")
		float cppWeaponYaw;

	UPROPERTY(EditAnywhere, Category = "Knight state")
		float baseAimSpaceValueRange;

	UPROPERTY(EditAnywhere, Category = "Knight state")
		float maxAimSpaceValueRange;
	
	UPROPERTY(EditAnywhere, Category = "Knight state")
		float baseWeaponSensitivty;

	//used for stagger
	UPROPERTY(EditAnywhere, BlueprintReadWrite , Category = "Knight state")
		bool isAllowedMaxBlendSpace;

	//modifiers for mouse move speed
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Knight state")
		float yawInputSensitivity;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Knight state")
		float pitchInputSensitivity;

	UFUNCTION(BlueprintCallable, Category = "Combat")
		void Die(AKnightBase* damageInstigator = nullptr, EDamageType damageType = EDamageType::DT_None);

	UFUNCTION(BlueprintCallable, Category = "Combat")
		void RoundFinish();
	
	//rotates character to target
	void LookAtTarget();

	//checks if opponent in in view
	bool CanSee();
	
	FTimerHandle loseSightTimer;

	float timeTillDropCombat;

	UFUNCTION(BlueprintCallable, Category = "Combat")
		void DropCombat();

	UFUNCTION(BlueprintCallable, Category = "Combat")
		void ToggleCombat();

	UFUNCTION(BlueprintCallable, Category = "Combat")
		void EnterCombat();

	UFUNCTION(BlueprintCallable, Category = "Combat")
		AKnightBase* FindEnemy();

	UFUNCTION(BlueprintCallable, Category = "Combat")
		bool isInRange();

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Knight LockOn")
		bool isLockedOn;

	float combatRange;

	//Timer data for easing in to lockon
	float currentLockOnSpeed;

	float beginLockOnSpeed;
	float endLockOnSpeed;
	float lockOnSpeedGain;
	float switchLockonSpeedTimeSeconds;

	FTimerHandle LockonTimer;

	void SwitchLockOnSpeeds();
	//////////////////////////////
	/*Network functions*/
	//////////////////////////////
	UFUNCTION(Reliable, Server, WithValidation)
	void SERVER_SetCombatState(ECombatState state);
	void SERVER_SetCombatState_Implementation(ECombatState state);
	bool SERVER_SetCombatState_Validate(ECombatState state);

	UFUNCTION(Reliable, Server, WithValidation)
	void SERVER_SetIsLockedOn(bool LockedOn);
	void SERVER_SetIsLockedOn_Implementation(bool LockedOn);
	bool SERVER_SetIsLockedOn_Validate(bool LockedOn);
	

	UFUNCTION(Reliable, Server, WithValidation)
	void ReplicateRotation(FRotator rotation);
	void ReplicateRotation_Implementation(FRotator rotation);
	bool ReplicateRotation_Validate(FRotator rotation);

	UFUNCTION(Reliable, Server, WithValidation)
	void SERVER_SetYaw(float pitch);
	void SERVER_SetYaw_Implementation(float pitch);
	bool SERVER_SetYaw_Validate(float pitch);

	UFUNCTION(Reliable, Server, WithValidation)
	void SERVER_SetPitch(float pitch);
	void SERVER_SetPitch_Implementation(float pitch);
	bool SERVER_SetPitch_Validate(float pitch);

	UFUNCTION(Reliable, NetMulticast, WithValidation)
	void NET_SetYaw(float yaw);
	void NET_SetYaw_Implementation(float yaw);
	bool NET_SetYaw_Validate(float yaw);

	UFUNCTION(Reliable, NetMulticast, WithValidation)
	void NET_SetPitch(float pitch);
	void NET_SetPitch_Implementation(float pitch);
	bool NET_SetPitch_Validate(float pitch);

	FRotator CalculateYawRotation();

	UFUNCTION()
	virtual void OnOverlapBegin(class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnOverlapEnd(class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	/** get current State of player */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Knight state")
	ECombatState combatState;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Knight state")
	ECombatState GetCurrentCombatState();

	UFUNCTION(BlueprintImplementableEvent)
	void onDeathEvent();

	UFUNCTION(Reliable, NetMulticast, WithValidation, Category = "Combat")
	void NET_OnDeath(EDamageType damageType);
	void NET_OnDeath_Implementation(EDamageType damageType);
	bool NET_OnDeath_Validate(EDamageType damageType);

	bool isADismemberment(FName boneName);

	UFUNCTION(Reliable, NetMulticast, WithValidation, Category = "Combat")
	void NET_Dismember(const FName boneName = NAME_None);
	void NET_Dismember_Implementation(const FName boneName = NAME_None);
	bool NET_Dismember_Validate(const FName boneName = NAME_None);


	void OnStrike(FSwordCollisionInfo SCI, ECollisionType collisionType);

	UFUNCTION(Reliable, Server, WithValidation,BlueprintCallable, Category = "Combat")
	void SERVER_OnHittingArmor(UBaS_ArmorBaseComponent* armorPiece, float damage, FVector hitLocation, AKnightBase* damageInstigator);
	void SERVER_OnHittingArmor_Implementation(UBaS_ArmorBaseComponent* armorPiece,float damage, FVector hitLocation, AKnightBase* damageInstigator);
	bool SERVER_OnHittingArmor_Validate(UBaS_ArmorBaseComponent* armorPiece,float damage, FVector hitLocation, AKnightBase* damageInstigator);

	UFUNCTION(Reliable, Server, WithValidation, BlueprintCallable, Category = "Combat")
	void SERVER_OnHittingSword(AWeaponBase* hitSword, float damage, FVector hitLocation);
	void SERVER_OnHittingSword_Implementation(AWeaponBase* hitSword, float damage, FVector hitLocation);
	bool SERVER_OnHittingSword_Validate(AWeaponBase* hitSword, float damage, FVector hitLocation);

	UFUNCTION(Reliable, Server, WithValidation, BlueprintCallable, Category = "Combat")
	void SERVER_OnHittingFlesh(AKnightBase* hitKnight, const float damage, const FVector &hitLocation, const FName boneName = NAME_None, AKnightBase* damageInstigator = nullptr);
	void SERVER_OnHittingFlesh_Implementation(AKnightBase* hitKnight, const float damage, const FVector &hitLocation, const FName boneName = NAME_None, AKnightBase* damageInstigator = nullptr);
	bool SERVER_OnHittingFlesh_Validate(AKnightBase* hitKnight, const float damage, const FVector &hitLocation, const FName boneName = NAME_None, AKnightBase* damageInstigator = nullptr);

	UFUNCTION(BlueprintImplementableEvent)
	void OnSwordStrike(	
		const AActor* opponentSword,
		const int32& mySwordSocket,
		const FVector& impactLocation,
		const int32& opponentSwordSocket);
	UFUNCTION(BlueprintImplementableEvent)
		void OnArmorStrike(
		const int32& mySwordSocket,
		const UBaS_ArmorBaseComponent* armorPiece,
		const FVector& impactLocation);
	UFUNCTION(BlueprintImplementableEvent)
		void OnFleshStrike(
		const int32& mySwordSocket,
		const AKnightBase* opponentKnight,
		const FName& OpponentBoneHit,
		const FVector& impactLocation);
	

	//Map<FName attachedToBoneName, ABaS_ArmorBaseComponent armorPiece> ArmorLookupMap;
	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TMap<FName, UBaS_ArmorBaseComponent*> ArmorMap;


	UFUNCTION(BlueprintCallable, Category = "Combat")
	UBaS_ArmorBaseComponent* ArmorLookUp(FName boneName);

	UFUNCTION(BlueprintCallable, Category = "Combat")
		void OnHit(const float damage, const FVector &hitLocation, const FName boneName = NAME_None, AKnightBase* damageInstigator = nullptr);

protected:

	
	void LookUpAtRate(float Rate);

	/**
	* Called via input to turn at a given rate.
	* @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	*/
	void TurnAtRate(float Rate);

	/**
	Called via input for character to look or move sword horizontally based on combatState
	*/
	void InputHorizontalAxis(float Value);

	/**
	called via input for character to look up or move sword vertically, based on combat state
	*/
	void InputVerticalAxis(float Value);

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	// End of APawn interface



	//*******//
	//Stamina//
	//*******//


	//Stamina

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
	bool bStaminaRegenAllowed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stamina")
	float maxStamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
	float staminaRegenRate;

	UPROPERTY(BlueprintReadWrite, Category = "Stamina")
	float stamina;

	void RegenerateStamina();

	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void DecreaseStamina(float amount);

	void OpenMenu();

	

	/*
	FeedBack Section
	*/

	UFUNCTION(BlueprintImplementableEvent)
	void PlayPlayerDeathSound(EDamageType damageType = EDamageType::DT_None, FName boneHit = NAME_None);

	UFUNCTION(BlueprintImplementableEvent)
	void PlayPlayerSpawnSound();

	UFUNCTION(BlueprintImplementableEvent)
	void PlayerOnHitFeedBack(float damage, EDamageType damageType, const FVector &hitLocation);

	UFUNCTION(BlueprintImplementableEvent)
	void PlayPlayerTauntSound();

	UFUNCTION(BlueprintImplementableEvent)
		void PlayerMeetEvent(AKnightBase* enemy);

	UFUNCTION(BlueprintImplementableEvent)
		void PlayerNearDeathEvent();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FeedBack")
	TArray<USoundCue*> TauntSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FeedBack")
	TArray<USoundCue*> OnHitSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FeedBack")
	TArray<USoundCue*> OnHitVoiceSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FeedBack")
	TArray<USoundCue*> OnDeathSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FeedBack")
	TArray<UParticleSystem*> BloodParticles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FeedBack")
	TArray<TSubclassOf<class UCameraShake>> OnHitArmorCameraShakes;

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateBloodyMesh(float healthScalar);

	//plyaers sound in knights voiceCOmponent on all clients
	UFUNCTION(Unreliable, NetMulticast, BlueprintCallable, Category = "Knight Sounds")
	void NET_Talk(USoundBase* sound, int32 talkPriority = 0);
	void NET_Talk_Implementation(USoundBase* sound, int32 talkPriority = 0);

	UFUNCTION(Reliable, NetMulticast, WithValidation, Category = "Combat")
	void NET_OnHitFeedback(const FVector &hitLocation, const float &forceOfhit, const FName boneName, AKnightBase* damageInstigator);
	void NET_OnHitFeedback_Implementation(const FVector &hitLocation, const float &forceOfhit, const FName boneName, AKnightBase* damageInstigator);
	bool NET_OnHitFeedback_Validate(const FVector &hitLocation, const float &forceOfhit, const FName boneName, AKnightBase* damageInstigator);
	
	UFUNCTION()
	void TalkDone();
	
	void UpdatePlayerStates(const AKnightBase* damageInstigator = nullptr);

public:
		
	UPROPERTY(Replicated,VisibleAnywhere, BlueprintReadWrite, Category = "Knight State")
	EKnightState currentKnightState;


	UFUNCTION(BlueprintCallable, Category = "FeedBack")
	void PlayCameraShake(const int32 typeOfHit);

	UFUNCTION(Reliable, NetMulticast, WithValidation, Category = "Combat")
	void NET_UpdateArmorUI(FName boneName, float healthScalar);
	void NET_UpdateArmorUI_Implementation(FName boneName, float healthScalar);
	bool NET_UpdateArmorUI_Validate(FName boneName, float healthScalar);

		//called to update the paperdoll of this actor
	UFUNCTION(BlueprintImplementableEvent)
	void InstigatorOnHitFeedback(EDamageType type);

	UFUNCTION(BlueprintImplementableEvent)
		void EnemyPlayerKilledEvent();

		//called to update the paperdoll of this actor
	UFUNCTION(BlueprintImplementableEvent)
	void ArmorUIEvent(FName boneName, float healthScalar);

		//called when an armor piece hits 50%
	UFUNCTION(BlueprintImplementableEvent)
	void ArmorHitEvent();

		//called when a armor piece breaks
	UFUNCTION(BlueprintImplementableEvent)
	void ArmorBrokeEvent(UBaS_ArmorBaseComponent* brokenPiece);

	//called when a armor piece breaks
	UFUNCTION(BlueprintImplementableEvent)
		void OnDestroyingArmorEvent(UBaS_ArmorBaseComponent* brokenPiece);

	
	UFUNCTION(BlueprintImplementableEvent)
		void LowHealthEvent();

	UFUNCTION(BlueprintImplementableEvent)
		void LongFightEvent();

	void CheckIfLongFight();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound Events")
	int32 longFightDurationInSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound Events")
	int32 quickHitTimeInSeconds;
	
	int32 lastHitOpponentTime;
	void CheckIfQuickHit();

	UFUNCTION(BlueprintImplementableEvent)
		void QuickHitEvent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound Priority")
	int32 DeathSoundPriority;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound Priority")
	int32 HitSoundPriority;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound Priority")
	int32 TalkPriortity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound Priority")
	int32 SwingSoundPriorty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound Priority")
	int32 LastSoundPriority;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Momentum")
	float pitchMomentum;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Momentum")
	float yawMomentum;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Momentum")
	float maxMomentum;
	//Value that shrinks input so momentum gain is smaller default 10
	//the smaller then number the faster top speed is reached
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Momentum")
	float momentumInputOffset;

	UFUNCTION(BlueprintCallable, Category = "Debug Tools")
	void KeyedDebugPrint(int32 key, float duration, FColor color, FString message);


	//scoreboard data/functionality

	UFUNCTION(BlueprintCallable, Category = "Score")
	ABaS_PlayerState* GetBaSPlayerState();

	int32 TimeOfSpawn;
	int32 TimeOfDeath;

	int32 CalculateLifeDuration();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ForceSwordStrike(AWeaponBase* opponentSword, int32 ownSwordSocket, FVector impactPoint, int32 opponentSwordSocket);
};


