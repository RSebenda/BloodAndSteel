// Fill out your copyright notice in the Description page of Project Settings.

#include "BloodAndSteel.h"
#include "KnightBase.h"
#include "Engine.h"
#include "Net/UnrealNetwork.h"
#include "BaS_PlayerController.h"
#include "BaS_PlayerState.h"


#pragma region Initialization
//////////////////////////////////////////////////////////////////////////
// ABloodAndSteelCharacter
//Replicated variables
void AKnightBase::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKnightBase, bIsAlive);
	DOREPLIFETIME(AKnightBase, IsHelmetOn);
	DOREPLIFETIME(AKnightBase, Health);
	DOREPLIFETIME(AKnightBase, combatState);
	DOREPLIFETIME_CONDITION(AKnightBase, cppWeaponPitch, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AKnightBase, cppWeaponYaw, COND_SkipOwner);
	DOREPLIFETIME(AKnightBase, ownWeapon);
	DOREPLIFETIME(AKnightBase, isLockedOn);
	DOREPLIFETIME_CONDITION(AKnightBase, currentKnightState, COND_SkipOwner);

}
//Constructor
AKnightBase::AKnightBase()
{
	deathTime = 5.0f;
	respawnTime = 2.0f;
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	//Create audio component for voice
	voiceComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("VoiceComponent"));
	//voiceComponent->AttachTo(GetMesh(), "headSocket");

	//Create particle system component
	bloodPSC = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("BloodParticle"));
	bloodPSC->AttachTo(GetMesh(), "headSocket");
	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	//FirstPersonCameraComponent->RelativeLocation = FVector(0, 0, 96.f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = false;
	FirstPersonCameraComponent->bEditableWhenInherited = true;
	FirstPersonCameraComponent->AttachTo(GetMesh(), "headSocket",EAttachLocation::SnapToTargetIncludingScale);

	voiceComponent->AttachTo(FirstPersonCameraComponent);
	//init lockon area
	lockOnTriggerArea = CreateDefaultSubobject<USphereComponent>(TEXT("Lock On Sphere"));
	lockOnTriggerArea->InitSphereRadius(500.0f);
	lockOnTriggerArea->AttachTo(RootComponent);
	lockOnTriggerArea->OnComponentBeginOverlap.AddDynamic(this, &AKnightBase::OnOverlapBegin);

	currentTarget = NULL;
	combatState = ECombatState::CS_FreeCam;
	combatWith = NULL;
	cppWeaponPitch = weaponPitchBase;
	cppWeaponYaw = weaponPitchBase;

	baseWeaponSensitivty = 100.0f;

	isLockedOn = true;
	combatRange = 2000.0f;
	beginLockOnSpeed = 0.1f;
	endLockOnSpeed = 1.0f;
	lockOnSpeedGain = 0.1f;
	switchLockonSpeedTimeSeconds =  0.4f;
	currentLockOnSpeed = beginLockOnSpeed;
	timeTillDropCombat = 2.0f;

	baseAimSpaceValueRange = 500;
	maxAimSpaceValueRange = 1000;
	isAllowedMaxBlendSpace = false;

	yawInputSensitivity = 1;
	pitchInputSensitivity = 1;

	ownWeapon = nullptr;

	Health = BASE_HEALTH;
	dismembermentThreshold = 90;
	lowHealthThreshold = 15;
	
	//Stamina
	maxStamina = 100;
	stamina = maxStamina;
	staminaRegenRate = 0.38f;
	bStaminaRegenAllowed = true;

	//Momentum
	pitchMomentum = 0;
	yawMomentum = 0;
	maxMomentum = 8;
	momentumInputOffset = 5;


	//NYI
	//voiceComponent->OnAudioFinished.AddDynamic(this, &AKnightBase::TalkDone);


	IsHelmetOn = true;

	int32 DeathSoundPriority = 100;
	int32 HitSoundPriority = 50;
	int32 TalkPriortity = 20;
	int32 SwingSoundPriorty = 10;
	int32 LastSoundPriority = 0;

	TimeOfSpawn = 0;
	lastHitOpponentTime = 0;
	//in seconds
	quickHitTimeInSeconds = 4;
	longFightDurationInSeconds = 300;
	currentKnightState = EKnightState::KS_VerticalGuard;
}

// Called when the game starts
void AKnightBase::BeginPlay()
{
	Super::BeginPlay();
	voiceComponent->SetBoolParameter("IsHelmetOn", true);
	TimeOfSpawn = FDateTime::UtcNow().ToUnixTimestamp();


	
	InitializeState();
	FirstPersonCameraComponent->bUsePawnControlRotation = false;

	stamina = maxStamina;

	if (Role == ROLE_Authority)
	{
		//Delay to spawn the sword

		FTimerHandle timer;
		GetWorldTimerManager().SetTimer(timer, this, &AKnightBase::SpawnOwnSword, 0.5f, false);
	}
}
//Only executed on the server when this actor gets possessed

void AKnightBase::SpawnOwnSword()
{
	if (Role == ROLE_Authority)
	{
		FActorSpawnParameters spawnParams;
		spawnParams.Owner = this;
		ABaS_PlayerController* pController = Cast<ABaS_PlayerController>(GetController());
		if (pController != nullptr)
		{
			AWeaponBase* newSword= GetWorld()->SpawnActor<AWeaponBase>(pController->weaponBP, GetActorLocation(), GetActorRotation(),spawnParams);
			newSword->AttachRootComponentTo(GetMesh(), FName(TEXT("SwordHand")), EAttachLocation::Type::SnapToTarget, true);
			ownWeapon = newSword;
			
			ownWeapon->ownerKnight = this;
			OnDestroyed.AddDynamic(ownWeapon, &AWeaponBase::OnKnightDestruction);

			
		}
	}
}
void AKnightBase::InitializeState()
{
	if (Role == ROLE_Authority)
	{
		Health = BASE_HEALTH;
		bIsAlive = true;
	}
}

#pragma endregion


void AKnightBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);



	//auto lock onto target if in range
	LookAtTarget();
	
	//if (Role == ROLE_Authority)
	
	if (bStaminaRegenAllowed && stamina<maxStamina)
	{
		RegenerateStamina();
	}
	
	
	//lower momentum everyframe
	//yawMomentum = FMath::FInterpTo(yawMomentum, 0, GetWorld()->DeltaTimeSeconds, 2);
	//pitchMomentum = FMath::FInterpTo(pitchMomentum, 0, GetWorld()->DeltaTimeSeconds, 2);
}

#pragma region Combat

void AKnightBase::OnOverlapBegin(class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AKnightBase* opponent = Cast<AKnightBase>(OtherActor);
	//if overlap is a Knight but not self
	if (opponent != nullptr && opponent != this)
	{
		combatWith = opponent;

		//special event for checking one player meet sound	
		if (Role == ROLE_Authority && IsLocallyControlled())
		PlayerMeetEvent(opponent);

	}
}

void AKnightBase::OnOverlapEnd(class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

}
// iterates world to find a target
AKnightBase* AKnightBase::FindEnemy()
{
	for (TObjectIterator<AKnightBase> Itr; Itr; ++Itr)
	{		
		AKnightBase* knight = *Itr;
		if (knight != this && knight->bIsAlive){	
			currentTarget = knight;		
			
		}
	}
	return nullptr;
}

void AKnightBase::LookAtTarget()
{

	if (currentTarget == nullptr){
		FindEnemy();

		return;
	}

	//if target dead
	if (!currentTarget->bIsAlive)
	{
		DropCombat();
		currentTarget = nullptr;
		return;
	}
	//if lose sight
	if (!GetWorldTimerManager().IsTimerActive(loseSightTimer) && (!CanSee() || !isInRange()))
	{
		
		GetWorldTimerManager().SetTimer(loseSightTimer, this, &AKnightBase::DropCombat, timeTillDropCombat, false);
	}

	//check if in range and can see enemy
	if (isLockedOn && CanSee() && isInRange())
	{
		GetWorldTimerManager().ClearTimer(loseSightTimer);
		//if recently entered combat
		if (!GetWorldTimerManager().IsTimerActive(LockonTimer) && currentLockOnSpeed < endLockOnSpeed)
		{
			EnterCombat();

		}
		combatState = ECombatState::CS_InCombat;
		//look at player mesh incase of dismemberment
		FVector FacingVector = (currentTarget->GetMesh()->GetComponentLocation() - GetActorLocation());
		FacingVector = FVector(FacingVector.X, FacingVector.Y, 0);
		FacingVector.Normalize();
		FRotator TargetRotation = FacingVector.Rotation();
		TargetRotation = FMath::Lerp(GetActorRotation(), TargetRotation, currentLockOnSpeed);
		//TargetRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, GetWorld()->DeltaTimeSeconds, INTERPOLATEVALUEENTER);
		SetActorRotation(TargetRotation);
		ReplicateRotation(TargetRotation);
	
	}
	else{
		DropCombat();
	}
	
}
 //reset  when you leave combat
void AKnightBase::DropCombat()
{
	if (IsLocallyControlled() && currentTarget)
	{
		currentTarget->GetMesh()->SetRenderCustomDepth(false);
		for (auto armorPiece : currentTarget->ArmorMap)
		{
			armorPiece.Value->SetRenderCustomDepth(false);
		}
	}
	

	combatState = ECombatState::CS_FreeCam;
	currentLockOnSpeed = beginLockOnSpeed;
	GetWorldTimerManager().ClearTimer(LockonTimer);
	OnLockOnTargetStop();
}


//switches state on input
void AKnightBase::EnterCombat()
{
	if (combatState == ECombatState::CS_InCombat)
	{
		if (IsLocallyControlled())
		{
			currentTarget->GetMesh()->SetRenderCustomDepth(true);

			for (auto armorPiece : currentTarget->ArmorMap)
			{
				armorPiece.Value->SetRenderCustomDepth(true);
			}
		}
		
		currentLockOnSpeed = beginLockOnSpeed;
		GetWorldTimerManager().SetTimer(LockonTimer, this, &AKnightBase::SwitchLockOnSpeeds, switchLockonSpeedTimeSeconds, true);

		FirstPersonCameraComponent->bUsePawnControlRotation = false;
		OnLockTargetStart();
	}
}


bool AKnightBase::CanSee()
{
	//check if in sight
	if (currentTarget)
	{
		FVector target = currentTarget->GetActorLocation();
		FVector distVector = target - GetActorLocation();
		distVector.Normalize();
		FVector selfForward = GetActorForwardVector();
		selfForward.Normalize();
		float dot = FVector::DotProduct(distVector, selfForward);
		float angle = FMath::RadiansToDegrees(FMath::Acos(dot));
		

		//check if in view of player
		if (FirstPersonCameraComponent->FieldOfView / 2 > angle)
		{

			//check if not being blocked
			FHitResult fHit(ForceInit);
			FVector self = GetMesh()->GetSocketLocation("headSocket");
			FCollisionQueryParams CombatCollisionQuery(FName(TEXT("CombatTrace")), true, NULL);
			CombatCollisionQuery.bTraceComplex = true;
			CombatCollisionQuery.bTraceAsyncScene = false;
			CombatCollisionQuery.bReturnPhysicalMaterial = false;
			CombatCollisionQuery.AddIgnoredActor(ownWeapon);
			CombatCollisionQuery.AddIgnoredActor(this);
			FCollisionObjectQueryParams objparams(FCollisionObjectQueryParams::InitType::AllObjects);
		
			GetWorld()->LineTraceSingleByChannel(fHit, self, target, ECC_WorldStatic, CombatCollisionQuery);
			//nothing between self and target
			if (fHit.Actor == currentTarget || fHit.Actor == currentTarget->ownWeapon)
			{
				return true;
			}
		}
	}
	return false;
}

bool AKnightBase::isInRange()
{
	//check if in distance
	if ((currentTarget->GetActorLocation() - GetActorLocation()).Size() < combatRange)
	{
		return true;
	}
	return false;
}

FRotator AKnightBase::CalculateYawRotation()
{
	if (currentTarget == NULL)
	{
		return FRotator(0, 0, 0);
	}
	FVector FacingVector = (currentTarget->GetActorLocation() - GetActorLocation());
	FacingVector = FVector(FacingVector.X, FacingVector.Y, 0);
	FacingVector.Normalize();
	FRotator TargetRotation = FacingVector.Rotation();
	return TargetRotation;
}

void AKnightBase::ToggleCombat()
{
	SERVER_SetIsLockedOn(!isLockedOn);
}


//increase speed at which player looks at target to prevent jittering
void AKnightBase::SwitchLockOnSpeeds()
{
	//stop timer if over speed
	if (currentLockOnSpeed >= endLockOnSpeed)
	{
		currentLockOnSpeed = endLockOnSpeed;
		GetWorldTimerManager().ClearTimer(LockonTimer);
		return;
	}
	currentLockOnSpeed += lockOnSpeedGain;
}
#pragma endregion

#pragma region ReplicateFunctions

void AKnightBase::SERVER_SetIsLockedOn_Implementation(bool switchedLockOn)
{
	isLockedOn = switchedLockOn;
}

bool AKnightBase::SERVER_SetIsLockedOn_Validate(bool switchedLockOn)
{
	return true;
}

bool AKnightBase::SERVER_SetCombatState_Validate(ECombatState state)
{
	return true;
}

void AKnightBase::SERVER_SetCombatState_Implementation(ECombatState state)
{
	combatState = state;
}

void AKnightBase::SERVER_SetYaw_Implementation(float yaw)
{
	cppWeaponYaw = yaw;
}

bool AKnightBase::SERVER_SetYaw_Validate(float yaw)
{
	return true;
}

void AKnightBase::SERVER_SetPitch_Implementation(float pitch)
{
	cppWeaponPitch = pitch;
}

bool AKnightBase::SERVER_SetPitch_Validate(float yaw)
{
	return true;
}

void AKnightBase::NET_SetYaw_Implementation(float yaw)
{
	cppWeaponYaw = yaw;
}
bool AKnightBase::NET_SetYaw_Validate(float yaw)
{
	return true;
}
void AKnightBase::NET_SetPitch_Implementation(float pitch)
{
	cppWeaponPitch = pitch;
}
bool AKnightBase::NET_SetPitch_Validate(float pitch)
{
	return true;
}

ECombatState AKnightBase::GetCurrentCombatState()
{
	return combatState;
}

void AKnightBase::ReplicateRotation_Implementation(FRotator rotation)
{
	SetActorRotation(rotation);
}

bool AKnightBase::ReplicateRotation_Validate(FRotator rotation)
{
	return true;
}

#pragma endregion

//UI Menu handler
void AKnightBase::OpenMenu()
{
	ABaS_PlayerController* pController = Cast<ABaS_PlayerController>(GetController());
	if (pController)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, TEXT("Open Menu"));
		if (pController->inGameMenu->IsVisible())
		{
			FInputModeGameOnly gameModeInput;
			pController->SetInputMode(gameModeInput);
			pController->bShowMouseCursor = false;
			pController->inGameMenu->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			FInputModeGameAndUI gameUIModeInput;
			pController->SetInputMode(gameUIModeInput);
			pController->bShowMouseCursor = true;
			pController->inGameMenu->SetVisibility(ESlateVisibility::Visible);
		}
	}
}


#pragma region Input

//////////////////////////////////////////////////////////////////////////
// Input
void AKnightBase::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// Set up gameplay key bindings
	check(InputComponent);
	InputComponent->BindAction("OpenMenu", IE_Released, this, &AKnightBase::OpenMenu);
	InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	InputComponent->BindAxis("LookUpRate", this, &AKnightBase::LookUpAtRate);
	InputComponent->BindAxis("LookHorizontal", this, &AKnightBase::InputHorizontalAxis);
	InputComponent->BindAxis("LookVertical", this, &AKnightBase::InputVerticalAxis);
	InputComponent->BindAction("Target Lock", IE_Pressed, this, &AKnightBase::ToggleCombat);
}

void AKnightBase::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AKnightBase::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}
//based on state move char or sword (yaw)
void AKnightBase::InputHorizontalAxis(float Value)
{
	
	float addedInput = 0;
	float clampedValue = 0;
	//calculated range based on recoil
	float clampRange = 0;
	FRotator addedRot = FRotator::ZeroRotator;

	switch (combatState)
	{
	case ECombatState::CS_FreeCam:
		addedRot = GetActorRotation() + FRotator(0, Value, 0);
		SetActorRotation(addedRot);
		ReplicateRotation(addedRot);

		cppWeaponYaw = FMath::FInterpTo(cppWeaponYaw, weaponPitchBase, GetWorld()->GetDeltaSeconds(), INTERPOLATEVALUEEXIT);
		
		if (Role == ROLE_AutonomousProxy)
		{
			SERVER_SetYaw(FMath::FInterpTo(cppWeaponYaw, weaponPitchBase, GetWorld()->GetDeltaSeconds(), INTERPOLATEVALUEEXIT));
		}

		break;

	case ECombatState::CS_InCombat:
		Value *= yawInputSensitivity * GetWorld()->GetDeltaSeconds() * baseWeaponSensitivty;
		//yawMomentum = FMath::Clamp<float>(Value / momentumInputOffset + yawMomentum,-maxMomentum,maxMomentum);

		addedInput = Value+ cppWeaponYaw;
		
		clampRange = isAllowedMaxBlendSpace == false ? baseAimSpaceValueRange : maxAimSpaceValueRange;
		clampedValue = FMath::Clamp(addedInput, -clampRange , clampRange);
		//TODO: Comment cpp yaw set directly only should be done in the server
		cppWeaponYaw = clampedValue;
		if (Role == ROLE_AutonomousProxy)
		{
			SERVER_SetYaw(clampedValue);
		}
		
		break;

	default:
		break;
		//do nothing!
	}
}

void AKnightBase::InputVerticalAxis(float Value)
{
	
	float addedInput = 0;
	float clampedValue = 0;
	//calculated range based on recoil
	float clampRange = 0;
	FRotator camRot = FRotator::ZeroRotator;
	FRotator socketRot;

	switch (combatState)
	{
	case ECombatState::CS_FreeCam:
		//Not in combat, move camera
		camRot = FirstPersonCameraComponent->GetComponentRotation();
		camRot.Pitch = FMath::Clamp(camRot.Pitch - Value, -MAX_CAMERA_ROTATION, MAX_CAMERA_ROTATION);
		//camRot = FMath::RInterpTo(camRot, GetActorRotation(), GetWorld()->GetDeltaSeconds(), INTERPOLATEVALUEENTER);
		camRot.Roll = FMath::FInterpTo(camRot.Roll, 0, GetWorld()->GetDeltaSeconds(), INTERPOLATEVALUEENTER);

		FirstPersonCameraComponent->SetWorldRotation(camRot);
		
		//TODO: Comment cpp pitch set directly only should be done in the server
		cppWeaponPitch = FMath::FInterpTo(cppWeaponPitch, weaponPitchBase, GetWorld()->GetDeltaSeconds(), INTERPOLATEVALUEEXIT);
		if (Role == ROLE_AutonomousProxy)
		{
			SERVER_SetPitch(FMath::FInterpTo(cppWeaponPitch, weaponPitchBase, GetWorld()->GetDeltaSeconds(), INTERPOLATEVALUEEXIT));
		}

		break;

	case ECombatState::CS_InCombat:
		//tuning for combat sword speed
		Value *= pitchInputSensitivity * GetWorld()->GetDeltaSeconds() * baseWeaponSensitivty;
		//pitchMomentum = FMath::Clamp<float>(Value/momentumInputOffset + pitchMomentum,-maxMomentum,maxMomentum);
		// regular
		addedInput = -Value+ cppWeaponPitch; 

		//in combat move sword/hands up/down
		
		clampRange = isAllowedMaxBlendSpace == false ? baseAimSpaceValueRange : maxAimSpaceValueRange;
		clampedValue = FMath::Clamp(addedInput, -clampRange, clampRange);

		//TODO: Comment cpp yaw set directly only should be done in the server
		cppWeaponPitch = clampedValue;
		if (Role == ROLE_AutonomousProxy)
		{
			SERVER_SetPitch(clampedValue);
		}


		//interp cam back to current Socket Rotation
		camRot = FirstPersonCameraComponent->GetComponentRotation();
		socketRot = GetMesh()->GetSocketRotation("headSocket");

		camRot.Pitch = FMath::FInterpTo(camRot.Pitch, socketRot.Pitch ,GetWorld()->GetDeltaSeconds(), INTERPOLATEVALUEENTER);
		camRot.Roll = FMath::FInterpTo(camRot.Roll, 0, GetWorld()->GetDeltaSeconds(), INTERPOLATEVALUEENTER);

		FirstPersonCameraComponent->SetWorldRotation(camRot);
		break;

	default:
		//do nothing!
		break;
	}

}

#pragma endregion

#pragma region StrikeHandling

void AKnightBase::ForceSwordStrike(AWeaponBase* opponentSword,int32 ownSwordSocket, FVector impactPoint, int32 opponentSwordSocket)
{
	OnSwordStrike(opponentSword,ownSwordSocket,impactPoint,opponentSwordSocket);
}

void AKnightBase::SERVER_OnHittingArmor_Implementation(UBaS_ArmorBaseComponent* armorPiece, float damage, FVector hitLocation, AKnightBase* damageInstigator)
{
	armorPiece->OnHit(damage, hitLocation, damageInstigator);
}
bool AKnightBase::SERVER_OnHittingArmor_Validate(UBaS_ArmorBaseComponent* armorPiece, float damage, FVector hitLocation, AKnightBase* damageInstigator)
{
	return true;
}
void AKnightBase::SERVER_OnHittingSword_Implementation(AWeaponBase* hitSword, float damage, FVector hitLocation)
{
	hitSword->OnHit(hitLocation,damage);
}
bool AKnightBase::SERVER_OnHittingSword_Validate(AWeaponBase* hitSword, float damage, FVector hitLocation)
{
	return true;
}
void AKnightBase::SERVER_OnHittingFlesh_Implementation(AKnightBase* hitKnight, const float damage, const FVector &hitLocation, const FName boneName, AKnightBase* damageInstigator)
{
	hitKnight->OnHit(damage,hitLocation,boneName,damageInstigator);
}
bool AKnightBase::SERVER_OnHittingFlesh_Validate(AKnightBase* hitKnight, const float damage, const FVector &hitLocation, const FName boneName, AKnightBase* damageInstigator)
{
	return true;
}
void AKnightBase::OnStrike(FSwordCollisionInfo SCI, ECollisionType collisionType)
{
	switch (collisionType)
	{
		case ECollisionType::CT_Weapon:
			OnSwordStrike(
				SCI.opponentSword,
				SCI.ownSwordSocket,
				SCI.impactPoint,
				SCI.opponentSwordSocket);
			break;
		case ECollisionType::CT_Armor:
			OnArmorStrike(
				SCI.ownSwordSocket,
				SCI.armorPiece,
				SCI.impactPoint);
			break;
		case ECollisionType::CT_Knight:
			OnFleshStrike(
				SCI.ownSwordSocket,
				SCI.opponent,
				SCI.boneHit,
				SCI.impactPoint);
			break;
		default:

			break;
	}
}
/*
bool AKnightBase::SERVER_OnStrike_Validate(FSwordCollisionInfo SCI, ECollisionType collisionType)
{
	return true;
}*/
//retunrs armor piece if bone has attached piece
UBaS_ArmorBaseComponent* AKnightBase::ArmorLookUp(FName boneName)
{
	if (ArmorMap.Contains(boneName)){
		UBaS_ArmorBaseComponent* hitPiece = *(ArmorMap.Find(boneName));

		return hitPiece;
	}
		return NULL;	
}

//when knight gets hit, deal damage and show feedback
void AKnightBase::OnHit(const float damage, const FVector &hitLocation, const FName boneName, AKnightBase* damageInstigator)
{
	if (Role == ROLE_Authority && bIsAlive)
	{
		ABaS_PlayerState* pState = GetBaSPlayerState();
		ABaS_PlayerState* InstPState = damageInstigator->GetBaSPlayerState();
		if (pState)
		{
			pState->DamageReceivedCount += damage;
		}
		if (InstPState)
		{
			InstPState->DamageDealtCount += damage;
		}

		EDamageType damageType = UStaticLibrary::CalculateDamageType(damage);
		NET_OnHitFeedback(hitLocation, damage, boneName, damageInstigator);
		Health -= damage;
		
		damageInstigator->CheckIfQuickHit();
		damageInstigator->CheckIfLongFight();

		if (Health <= lowHealthThreshold && Health > 0)
		{
			damageInstigator->LowHealthEvent();
		}
		if (damage > dismembermentThreshold)
		{
			Health = 0;

		}
		if (Health <= 0)
		{
			if (isADismemberment(boneName))
				InstPState->AddDismemberment(boneName);


			Die(damageInstigator, damageType);
			NET_Dismember(boneName);
		}

	}
}
#pragma endregion

#pragma region DeathHandling

void AKnightBase::Die(AKnightBase* damageInstigator, EDamageType damageType)
{
	bIsAlive = false;
	UpdatePlayerStates(damageInstigator);
	NET_OnDeath(damageType);
	damageInstigator->EnemyPlayerKilledEvent();

	//turn off character and respawn for next round
	damageInstigator->bIsAlive = false;
	damageInstigator->WinnerRespawn();
	
	TimeOfDeath = FDateTime::UtcNow().ToUnixTimestamp();
	GetBaSPlayerState()->AddSwordDuration(ownWeapon->GetSwordType(), CalculateLifeDuration());

	
	if (!GetWorldTimerManager().IsTimerActive(respawnHandle))
	{	
		GetWorldTimerManager().SetTimer(respawnHandle, this, &AKnightBase::Respawn, deathTime, false);
	}
}

//called PC to create new knight
void AKnightBase::Respawn()
{
	
	if (Role == ROLE_Authority)
	{	
		ABaS_PlayerController* pController = Cast<ABaS_PlayerController>(GetController());
		if (ownWeapon)
		{
			ownWeapon->Destroy();
		}
		Destroy();

		if (pController != nullptr)
		{
			pController->RequestRespawnPlayer(respawnTime);
		}
		
	}
}

//when knight who gets a kill needs to reset
void AKnightBase::WinnerRespawn()
{
	TimeOfDeath = FDateTime::UtcNow().ToUnixTimestamp();
	GetBaSPlayerState()->AddSwordDuration(ownWeapon->GetSwordType(), CalculateLifeDuration());
	if (!GetWorldTimerManager().IsTimerActive(respawnHandle))
	{
		GetWorldTimerManager().SetTimer(respawnHandle, this, &AKnightBase::Respawn, deathTime, false);
	}

}


//updates score
void AKnightBase::UpdatePlayerStates(const AKnightBase* damageInstigator )
{
	ABaS_PlayerState* pState = Cast<ABaS_PlayerState>(PlayerState);
	ABaS_PlayerController* pController = Cast<ABaS_PlayerController>(GetController());
	if (pState)
	{
		pState->DeathCount += 1;
	
		pState->OnRep_SetDeaths();
	}
	if (pController)
	{
		pController->CLIENT_AddDeath();

	}


	if (damageInstigator)
	{
		ABaS_PlayerState* instigatorPState = Cast<ABaS_PlayerState>(damageInstigator->PlayerState);
		ABaS_PlayerController* instigatorController = Cast<ABaS_PlayerController>(damageInstigator->GetController());
		if (instigatorPState)
		{
			instigatorPState->KillCount += 1;
			instigatorPState->OnRep_SetKills();
		}
		if (instigatorController)
		{
			instigatorController->CLIENT_AddKill();

		}


	}
}

//multicast to all players on play death
void AKnightBase::NET_OnDeath_Implementation(EDamageType damageType)
{
	ownWeapon->DetachRootComponentFromParent();
	ownWeapon->WeaponMesh->SetSimulatePhysics(true);
	onDeathEvent();
	PlayPlayerDeathSound(damageType);
	GetMesh()->SetAllBodiesSimulatePhysics(true);
}
bool AKnightBase::NET_OnDeath_Validate(EDamageType damageType)
{
	return true;
}

//checks if hit bone has armor attached
bool AKnightBase::isADismemberment(FName boneName)
{
	if (!boneName.ToString().Contains("spine") && ArmorMap.Contains(boneName))
		return true;

	return false;

}


void AKnightBase::NET_Dismember_Implementation(const FName boneName)
{
	//unhide head due to it being hidden to prevent camera clipping
	GetMesh()->UnHideBoneByName("head");

	//was a dismemberment
	if (!boneName.ToString().Contains("spine") && ArmorMap.Contains(boneName))
	{

		
		//hide armor if connected to this bone
		ArmorLookUp(boneName)->SetVisibility(false);

		GetMesh()->HideBoneByName(boneName, EPhysBodyOp::PBO_Disable);

		FName socketName = NAME_None;
		//UStaticLibrary::DebugMessage("dismember called!", FColor::Red, 5.0f);
		for (FName currentSocket : GetMesh()->GetAllSocketNames())
		{
			if (currentSocket.ToString().Contains(boneName.ToString()))
			{
				socketName = currentSocket;
				break;
			}

		}
		if (socketName != NAME_None)
		{
			//UStaticLibrary::DebugMessage("Activating blood!", FColor::Red, 5.0f);
			bloodPSC->AttachTo(GetMesh(), socketName);
			bloodPSC->ActivateSystem();

			//spawn limb on server, gets rep on clients
			if (Role == ROLE_Authority)
			{		
				FTransform limbTrans = FTransform( FRotator(0, 0, GetActorForwardVector().Rotation().Yaw) , GetMesh()->GetBoneLocation(boneName));
				AActor* limb = GetWorld()->SpawnActor<AActor>(UStaticLibrary::GetLimbs()[boneName], limbTrans);
			
			}


		}

		
	}
}
bool AKnightBase::NET_Dismember_Validate(const FName boneName)
{
	return true;
}

//called when a player dies to reset players
void AKnightBase::RoundFinish(){

	Respawn();

}

#pragma endregion 

#pragma region StaminaHandling
//Stamina Section


void AKnightBase::RegenerateStamina()
{
	if (stamina >= maxStamina)
	{
		stamina = maxStamina;
		return;
	}
	stamina += staminaRegenRate * GetWorld()->GetDeltaSeconds();
}

void AKnightBase::DecreaseStamina(float amount)
{
	if (stamina < amount)
	{
		stamina = 0;
	}
	else
	{
		stamina -= amount;
	}
}

#pragma endregion

#pragma region Feedback

void AKnightBase::NET_Talk_Implementation(USoundBase* sound, int32 talkPriority)
{
	
	if (!voiceComponent->IsPlaying())
	{
		
		voiceComponent->SetSound(sound);
		LastSoundPriority = talkPriority;
		voiceComponent->Play();
	}
	else{ //isplaying
		//check if new sound has more priority or if last and new sound onHitSound for special case
		if (talkPriority > LastSoundPriority || (TalkPriortity == HitSoundPriority && LastSoundPriority == HitSoundPriority))
		{
			voiceComponent->SetSound(sound);

			LastSoundPriority = talkPriority;
			voiceComponent->Stop();
			voiceComponent->Play();
		}

	}

}

//feedback for all players when knight was hit
void AKnightBase::NET_OnHitFeedback_Implementation(const FVector &hitLocation, const float &forceOfhit, const FName boneName,  AKnightBase* damageInstigator)
{
	EDamageType damageType = UStaticLibrary::CalculateDamageType(forceOfhit);
	PlayerOnHitFeedBack(forceOfhit, damageType, hitLocation);
	UpdateBloodyMesh( 1.0f - Health / BASE_HEALTH);

	if (damageInstigator)
	{
		damageInstigator->InstigatorOnHitFeedback(damageType);
	}
	ABaS_PlayerController* pc = Cast<ABaS_PlayerController>(GetController());

	if (pc){
		pc->OnFleshHitUIEvent(boneName);
	}

}

bool AKnightBase::NET_OnHitFeedback_Validate(const FVector &hitLocation, const float &forceOfhit, const FName boneName,  AKnightBase* damageInstigator)
{
	return true;
}


void AKnightBase::NET_UpdateArmorUI_Implementation(FName boneName, float healthScalar)
{

	ArmorUIEvent(boneName, healthScalar);
}
bool AKnightBase::NET_UpdateArmorUI_Validate(FName boneName, float healthScalar)
{

	return true;
}

void AKnightBase::TalkDone()
{
	LastSoundPriority = 0;
	
}

//only called on client
void AKnightBase::PlayCameraShake(const int typeOfHit) //TSubclassOf<class UCameraShake> shake)
{
	//check if none
	if (typeOfHit <= 0)
		return;

	APlayerController* apc = Cast<APlayerController>(Controller);
	if (apc)
	{
		if (OnHitArmorCameraShakes.Num() > typeOfHit && OnHitArmorCameraShakes[typeOfHit] != nullptr)
		{
			apc->ClientPlayCameraShake(OnHitArmorCameraShakes[typeOfHit], 1.0f);

		}

	}

}

//event called on hit if fight lasts over X duration
void AKnightBase::CheckIfLongFight()
{
	int32 timeNow = FDateTime::UtcNow().ToUnixTimestamp();

	if (timeNow - TimeOfSpawn > longFightDurationInSeconds)
	{
		LongFightEvent();
	}

	
}

//called on hit if knight hits rapidly
void AKnightBase::CheckIfQuickHit()
{
	int32 timeNow = FDateTime::UtcNow().ToUnixTimestamp();
	
	if (quickHitTimeInSeconds < timeNow - lastHitOpponentTime)
	{
		quickHitTimeInSeconds;
	}

	lastHitOpponentTime = timeNow;
}

#pragma endregion


void AKnightBase::KeyedDebugPrint(int32 key, float duration, FColor color, FString message)
{
	GEngine->AddOnScreenDebugMessage(key, duration, color, message);
}


ABaS_PlayerState* AKnightBase::GetBaSPlayerState()
{

	return Cast<ABaS_PlayerState>(PlayerState);

}


int32 AKnightBase::CalculateLifeDuration()
{
	return TimeOfDeath - TimeOfSpawn;
	
}
