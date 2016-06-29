//Robert Sebenda, Manuel Cebreros
//Copyright 2016

#include "BloodAndSteel.h"
#include "KnightBase.h"
#include "BaS_ArmorBaseComponent.h"
#include "BaS_PlayerState.h"



void UBaS_ArmorBaseComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UBaS_ArmorBaseComponent, armorState);
	DOREPLIFETIME(UBaS_ArmorBaseComponent, armorHealth);
	DOREPLIFETIME(UBaS_ArmorBaseComponent, BrokenMeshComponent);
	DOREPLIFETIME(UBaS_ArmorBaseComponent, BrokenMesh);
}


UBaS_ArmorBaseComponent::UBaS_ArmorBaseComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;
	//BrokenMeshComponent = CreateDefaultSubobject<UDestructibleComponent>(TEXT("DestMesh Component"));
	armorDamagerModifer = 1.0f;
	BaseHealth = 50.0f;
	damagedHealthLevel = 25.0f;

	armorFlashLength = 1.0f;
	//init defaults of class , if this class was a blueprint it would be easier!
	struct FConstructorStatics
	{	
		ConstructorHelpers::FObjectFinder<UMaterial> dMat;
		ConstructorHelpers::FObjectFinder<USoundCue> armorHitSound_None;
		ConstructorHelpers::FObjectFinder<USoundCue> armorHitSound_light;
		ConstructorHelpers::FObjectFinder<USoundCue> armorHitSound_medium;
		ConstructorHelpers::FObjectFinder<USoundCue> armorHitSound_heavy;
		ConstructorHelpers::FObjectFinder<USoundCue> armorHitSound_dev;
		ConstructorHelpers::FObjectFinder<USoundCue> armorBreakSound;
	

		FConstructorStatics()
			: dMat(TEXT("/Game/Armor/RedTest.RedTest"))
			, armorHitSound_None(TEXT("/Game/Sound/Impacts/Armour/SFX_Hit_Armour_0.SFX_Hit_Armour_0"))
			, armorHitSound_light(TEXT("/Game/Sound/Impacts/Armour/SFX_Hit_Armour_1.SFX_Hit_Armour_1"))
			, armorHitSound_medium(TEXT("/Game/Sound/Impacts/Armour/SFX_Hit_Armour_2.SFX_Hit_Armour_2"))
			, armorHitSound_heavy(TEXT("/Game/Sound/Impacts/Armour/SFX_Hit_Armour_3.SFX_Hit_Armour_3"))
			, armorHitSound_dev(TEXT("/Game/Sound/Impacts/Armour/SFX_Hit_Armour_4.SFX_Hit_Armour_4"))
			, armorBreakSound(TEXT("/Game/Sound/Impacts/Armour/SFX_Hit_Armour_Break.SFX_Hit_Armour_Break"))
			//SoundCue'/Game/Sound/Impacts/Armour/SFX_Hit_Armour_0.SFX_Hit_Armour_0'
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	//static ConstructorHelpers::FObjectFinder<UMaterial> dMat(TEXT("/Game/Armor/RedTest.RedTest"));
	//static ConstructorHelpers::FObjectFinder<UParticleSystem> sparksTemplate(TEXT("/Game/Particles/Sparks.Sparks"));
	//static ConstructorHelpers::FObjectFinder<USoundBase> hitSound(TEXT("/Game/Sound/Impacts/Sword_Hit_Placeholder_Cue.Sword_Hit_Placeholder_Cue"));


	damagedMaterial = ConstructorStatics.dMat.Object;
	armorHitSounds.Add(ConstructorStatics.armorHitSound_None.Object);
	armorHitSounds.Add(ConstructorStatics.armorHitSound_light.Object);
	armorHitSounds.Add(ConstructorStatics.armorHitSound_medium.Object);
	armorHitSounds.Add(ConstructorStatics.armorHitSound_heavy.Object);
	armorHitSounds.Add(ConstructorStatics.armorHitSound_dev.Object);
	armorBreakSounds.Add(ConstructorStatics.armorBreakSound.Object);
	
	sparks = UStaticLibrary::GetSparks();

	noneEffect = 0;
	lightEffect = 1;
	mediumEffect = 2;
	HeavyEffect = 3;
	DevEffect = 4;

	ScalarCap = 1.0f;
	BaseUVScale = 3.0f;
	BaseDamageBlend = 0.0f;

}

void UBaS_ArmorBaseComponent::BeginPlay()
{
	//Super::BeginPlay();
	//set up dynam material
	dynamMat = CreateDynamicMaterialInstance(0, GetMaterial(0));
	dynamMat->SetScalarParameterValue("UVScaling", BaseUVScale);
	dynamMat->SetScalarParameterValue("DamageBlend", BaseDamageBlend);
	SetMaterial(0, dynamMat);

	armorHealth = BaseHealth;
	armorState = EArmorStatus::AS_Pristine;
	AddToArmorLookup();
}

// Called every frame
void UBaS_ArmorBaseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	//Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

//On begin add this to parent Knight
void UBaS_ArmorBaseComponent::AddToArmorLookup()
{
	FName socketName = this->AttachSocketName;
	AKnightBase* knight = Cast<AKnightBase>(GetOwner());
	FName boneName = knight->GetMesh()->GetSocketBoneName(socketName);

	knight->ArmorMap.Add(boneName, this);
}
void UBaS_ArmorBaseComponent::WrapperFunction(float damage, FVector hitLocation, AKnightBase* damageInstigator)
{
	SERVER_OnHit(damage,hitLocation,damageInstigator);
}
//deals damage to armor piece, updates the mesh at certain thresholds
void UBaS_ArmorBaseComponent::OnHit(float damage, FVector hitLocation,AKnightBase* damageInstigator)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		EArmorStatus stateBeforeHit = armorState;
		armorHealth -= damage * armorDamagerModifer;

		AKnightBase* owner = Cast<AKnightBase>(GetOwner());
		owner->GetBaSPlayerState()->DamageReceivedCount += damage;
		damageInstigator->GetBaSPlayerState()->DamageDealtCount += damage;

		//feedback
		EDamageType type = UStaticLibrary::CalculateDamageType(damage);
		owner->CheckIfQuickHit();

		UpdateArmorUI();
		NET_Armor_Feedback(type, hitLocation, armorHealth, damageInstigator);

		if (armorHealth <= 0)
		{
			armorState = EArmorStatus::AS_Broken;
		}
		else if (armorHealth <= damagedHealthLevel)
		{
			armorState = EArmorStatus::AS_Damaged;
		}
		//check if change
		if (armorState != stateBeforeHit && GetOwnerRole() == ROLE_Authority){
			//ReplicateUpdateArmorMesh(armorState, damage);
			switch (armorState)
			{
				case EArmorStatus::AS_Pristine:
					ReplicateUpdateArmorMesh(armorState, armorHealth);
					break;

				case EArmorStatus::AS_Damaged:
					ReplicateUpdateArmorMesh(armorState, armorHealth);
					break;

				case EArmorStatus::AS_Broken:
					SetCollisionEnabled(ECollisionEnabled::NoCollision);
					this->BrokenMeshComponent->SetWorldLocation(this->GetComponentLocation());
					ReplicateUpdateArmorMesh(armorState, armorHealth);
					damageInstigator->OnDestroyingArmorEvent(this);
					damageInstigator->GetBaSPlayerState()->ArmorPiecesBroken++;
					break;

				default:
					break;
			}	
		}
	}
}
//deprecated
void UBaS_ArmorBaseComponent::SERVER_OnHit_Implementation(float damage, FVector hitLocation, AKnightBase* damageInstigator)
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Armor Hit Server");
	EArmorStatus stateBeforeHit = armorState;
	armorHealth -= damage * armorDamagerModifer;

	//feedback
	EDamageType type = UStaticLibrary::CalculateDamageType(damage);


	UpdateArmorUI();
	//EDamageType typeofHit, const FVector hitLocation, const float &health, AKnightBase* damageInstigator = nullptr
	//NET_Armor_Feedback(armorHitSounds[typeInt], sparks[typeInt], hitLocation, armorHealth, typeInt, damageInstigator);
	NET_Armor_Feedback(type, hitLocation, armorHealth, damageInstigator);

	if (armorHealth <= 0)
	{
		armorState = EArmorStatus::AS_Broken;
		damageInstigator->OnDestroyingArmorEvent(this);
	}
	else if (armorHealth <= damagedHealthLevel)
	{
		armorState = EArmorStatus::AS_Damaged;
	}
	//check if change
	if (armorState != stateBeforeHit && GetOwnerRole() == ROLE_Authority){
		//ReplicateUpdateArmorMesh(armorState, damage);
		switch (armorState)
		{
		case EArmorStatus::AS_Pristine:
			ReplicateUpdateArmorMesh(armorState, armorHealth);
			break;

		case EArmorStatus::AS_Damaged:
			ReplicateUpdateArmorMesh(armorState, armorHealth);
			break;

		case EArmorStatus::AS_Broken:
		{
			SetCollisionEnabled(ECollisionEnabled::NoCollision);
			this->BrokenMeshComponent->SetWorldLocation(this->GetComponentLocation());
			ReplicateUpdateArmorMesh(armorState, armorHealth);

			
		}
		break;

		default:
			break;
		}
	}
}
bool UBaS_ArmorBaseComponent::SERVER_OnHit_Validate(float damage, FVector hitLocation, AKnightBase* damageInstigator)
{
	return true;
}

void UBaS_ArmorBaseComponent::NET_Armor_Feedback_Implementation(EDamageType typeOfHit, const FVector hitLocation, const float &health, AKnightBase* damageInstigator)
{

	int32 typeInt = 0;

	switch (typeOfHit)
	{
	case EDamageType::DT_None:
		typeInt = noneEffect;
		break;
	case EDamageType::DT_Light:
		typeInt = lightEffect;
		break;
	case EDamageType::DT_Medium:
		typeInt = mediumEffect;
		break;
	case EDamageType::DT_Heavy:
		typeInt = HeavyEffect;
		break;
	case EDamageType::DT_Devastating:
		typeInt = DevEffect;
		break;
	}

	UGameplayStatics::PlaySoundAtLocation(GetWorld(), armorHitSounds[typeInt], hitLocation);
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), sparks[typeInt], hitLocation);
	AKnightBase* knight = Cast<AKnightBase>(GetOwner());
	
	//get damage scalar from armor health, clamp it and inverse to 0 -> 1
	float dmgScalar = health / BaseHealth;
	dmgScalar = 1.0f - FMath::Clamp(dmgScalar, 0.0f, 1.0f);
	
	dynamMat->SetScalarParameterValue("DamageBlend", dmgScalar);

	//only flash if you do damage
	if (typeOfHit != EDamageType::DT_None)
	{
		///clear flashing armor
		if (GetOwner()->GetWorldTimerManager().IsTimerActive(armorFlashHandle))
		{
			dynamMat->SetScalarParameterValue("Flashing", 0);
			GetOwner()->GetWorldTimerManager().ClearTimer(armorFlashHandle);

		}
		//start armor flash
		dynamMat->SetScalarParameterValue("Flashing", 2);
		GetOwner()->GetWorldTimerManager().SetTimer(armorFlashHandle, this, &UBaS_ArmorBaseComponent::TurnOffArmorFlash, armorFlashLength, false);
	}
	SetMaterial(0, dynamMat);
	if (knight)
	{
		
		knight->PlayCameraShake(typeInt);
	}
	//affect the knight who hit this
	if (damageInstigator)
	{
		damageInstigator->InstigatorOnHitFeedback(typeOfHit);
	}
}

void UBaS_ArmorBaseComponent::TurnOffArmorFlash()
{
	dynamMat->SetScalarParameterValue("Flashing", 0);
}



bool UBaS_ArmorBaseComponent::NET_Armor_Feedback_Validate(EDamageType typeofHit, const FVector hitLocation, const float &health, AKnightBase* damageInstigator)
{
	return true;
}

// to implement
void UBaS_ArmorBaseComponent::UpdateArmorMesh()
{

}
void UBaS_ArmorBaseComponent::ReplicateUpdateArmorMesh_Implementation(const EArmorStatus &armorState, const float &health)
{
	AKnightBase* knight = Cast<AKnightBase>(GetOwner());
	this->armorHealth = health;

	switch (armorState)
	{
	case EArmorStatus::AS_Pristine:
		break;
	case EArmorStatus::AS_Damaged:	
		//this->SetMaterial(0, damagedMaterial);
		if (knight)
			knight->ArmorHitEvent();
		break;
	case EArmorStatus::AS_Broken:
	{ //new scope for sound var
		//sound feedback
		int breakSound = FMath::RandRange(0, armorBreakSounds.Num() - 1);

		UGameplayStatics::PlaySoundAtLocation(GetWorld(), armorBreakSounds[breakSound], this->GetComponentLocation());
		
		if (knight)
		{
			knight->ArmorBrokeEvent(this);
			
			//check if helm broken and set var
			FName socketName = this->AttachSocketName;
			FName boneName = knight->GetMesh()->GetSocketBoneName(socketName);

			if (boneName.ToString().Contains("head"))
			{
				knight->IsHelmetOn = false;
				knight->voiceComponent->SetBoolParameter("IsHelmetOn", false);
				//UStaticLibrary::DebugMessage("helm off!");
			}

		}
	
		SetVisibility(false);
		SetCollisionEnabled(ECollisionEnabled::NoCollision);
		this->BrokenMeshComponent->SetWorldLocation(this->GetComponentLocation());
		this->BrokenMeshComponent->SetMaterial(0 , this->GetMaterial(0));
		this->BrokenMeshComponent->SetDestructibleMesh(BrokenMesh);
	}
		break;
	default:
		break;
	}
}

bool UBaS_ArmorBaseComponent::ReplicateUpdateArmorMesh_Validate(const EArmorStatus &armorState, const float &armorHealth)
{
	return true;
}

void UBaS_ArmorBaseComponent::UpdateArmorUI(){
	
	AKnightBase* knight = Cast<AKnightBase>(GetOwner());

	if (knight)
	{
		ABaS_PlayerController* pc = Cast<ABaS_PlayerController>(knight->GetController());

		if (pc)
		{
			float healthScalar = armorHealth / BaseHealth;
			FName boneName = knight->GetMesh()->GetSocketBoneName(AttachSocketName);
			//update my paperdoll in world
			knight->NET_UpdateArmorUI(boneName, healthScalar);

			//update my ui paperdoll
			pc->CLIENT_UpdateArmorUI(boneName, healthScalar);
		}
	}
}

