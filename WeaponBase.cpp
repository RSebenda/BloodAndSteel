//Robert Sebenda, Manuel Cebreros
//Copyright 2016

#include "BloodAndSteel.h"
#include "KnightBase.h"
#include "WeaponBase.h"
#include "BaS_ArmorBaseComponent.h"

void AWeaponBase::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeaponBase, ownerKnight);
	
}

// Sets default values
AWeaponBase::AWeaponBase(const FObjectInitializer& ObjectInitializer) :
Super(ObjectInitializer)
{
	bReplicates = true;
	bAlwaysRelevant = true;

	ownerKnight = nullptr;
	//static ConstructorHelpers::FObjectFinder<UParticleSystem> sparksTemplate(TEXT("/Game/Particles/Sparks.Sparks"));
	//static ConstructorHelpers::FObjectFinder<USoundBase> soundCue(TEXT("/Game/Sound/Impacts/Sword_Hit_Placeholder_Cue.Sword_Hit_Placeholder_Cue"));


	sparks = UStaticLibrary::GetSparks();
	//swordSound = soundCue.Object;
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	RootComponent = WeaponMesh;

	//defaults for weapon stats
	weaponBaseDamage = 10;
	weaponSpeedBase = 1;
	weaponInertia = 1;
	weaponRecoilModifier = 1;
	WeaponStaminaModifier = 1;
}

// Called when the game starts or when spawned
void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	cooldownTimer = 0;

	bIsOnCooldown = false;
	//init weapon parts
	velocity = 0;
	lastPosition = GetActorLocation();




	GetAllSockets();
}

// Called every frame
void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	
	if (bIsOnCooldown)
	{
		cooldownTimer += DeltaTime;
		//TODO: replace with collisionCooldown variable
		if (cooldownTimer > 0.5f)
		{
			cooldownTimer = 0;
			bIsOnCooldown = false;
		}
	}

	
	/*FVector start = sockets[2]->lastPosition;
	FVector end = sockets[0]->lastPosition;

	FHitResult HitData(ForceInit);
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(NULL);
	Trace(GetWorld(), GetPlayerControllerFromNetId(),start,end,);
	*/
	//get vel of whole sword
	velocity = (GetActorLocation() - lastPosition).Size();
	lastPosition = GetActorLocation();

	UpdateSocketData();
}

void AWeaponBase::UpdateVelocity(AActor swordPart)
{

	velocity = (GetActorLocation() - lastPosition).Size();
	lastPosition = GetActorLocation();

	  
}


void AWeaponBase::TestCollision(float partVelocity, AActor* otherActor)
{

	AWeaponBase* otherWeapon = Cast<AWeaponBase>(otherActor);

	//was a weapon
	if (otherWeapon)
	{
	}
}

void AWeaponBase::GetAllSockets()
{
	if (WeaponMesh != NULL)
	{
		TArray<FName> socketNames = WeaponMesh->GetAllSocketNames();	

		USocketData* temp;
		for (FName name : socketNames)
		{
			if (name.ToString().Contains("Socket"))
			{
				temp = NewObject<USocketData>();
				temp->AddSocket(name);
				sockets.Add(temp);				
			}
		}
	}
}

void AWeaponBase::UpdateSocketData()
{

	for (USocketData* socket : sockets)
	{
		
		socket->UpdateData(WeaponMesh->GetSocketLocation(socket->socketName));


		//gets tipSocket .. etc
		//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, socket->socketName.ToString());
	}
}
//This function gets called by blueprint only when the trace detected a hit
void AWeaponBase::OnCollisionDetected(FHitResult const &hitResult)
{	
	
	//Everything inside this if statement is executed on the server and only by the server
	//So everything gameplay important can be safely resolved here
	if (!bIsOnCooldown)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, "OnCollisionDetected");
		bIsOnCooldown = true;
		//Resolve collision
		//here
		ECollisionType collisionType;
		FSwordCollisionInfo SCI;

		SCI.ownSword = this;
		SCI.ownSwordSocket = currentSocketCast;
		SCI.boneHit = hitResult.BoneName;
		SCI.impactPoint = hitResult.ImpactPoint;
		
		AKnightBase* ownerKnight = Cast<AKnightBase>(GetOwner());

		AWeaponBase* enemySword = Cast<AWeaponBase>(hitResult.GetActor());
		UBaS_ArmorBaseComponent* armor = Cast<UBaS_ArmorBaseComponent>(hitResult.GetComponent());
		AKnightBase* enemyKnight = Cast<AKnightBase>(hitResult.GetActor());

		if (enemySword != nullptr)
		{
			collisionType = ECollisionType::CT_Weapon;
			//UStaticLibrary::DebugMessage(enemySword->GetName());
		}
		else if (armor != nullptr)
		{
			collisionType = ECollisionType::CT_Armor;
		}
		else if (enemyKnight != nullptr)
		{
			collisionType = ECollisionType::CT_Knight;
		}
		else
		{
			//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, "Invalid Target");
			bIsOnCooldown = true;
			return;
		}

		switch (collisionType)
		{
			case ECollisionType::CT_Weapon:
				SCI.opponentSword = enemySword;
				SCI.opponentSwordSocket = enemySword->currentSocketCast;
				ownerKnight->OnStrike(SCI, ECollisionType::CT_Weapon);
				break;
			case ECollisionType::CT_Armor:
				SCI.armorPiece = armor;
				ownerKnight->OnStrike(SCI, ECollisionType::CT_Armor);
				break;
			case ECollisionType::CT_Knight:
				SCI.opponent = enemyKnight;
				ownerKnight->OnStrike(SCI, ECollisionType::CT_Knight);
				break;
			default:
				break;
		}
	}
}
//Function for cosmetic purposes only. Nothing gameplay relevant since clients are gonna execute this
//And we don't want them cheating or going out of sync.
void AWeaponBase::NET_OnCollisionResolved_Implementation(ECollisionType collisionType, FVector impactPoint, FName bone)
{
	UWorld* world = GetWorld();
	
	switch (collisionType)
	{
		case ECollisionType::CT_Weapon:
			//PlaySparkHit(world,impactPoint);
			break;
		case ECollisionType::CT_Armor:
			//PlaySparkHit(world, impactPoint);
			break;
		case ECollisionType::CT_Knight:
			//PlaySparkHit(world, impactPoint);
			break;
		default:
			break;
	}
}


void AWeaponBase::OnHit(FVector hitLocation, float forceOfHit)
{
	if (Role == ROLE_Authority)
	{
		NET_Sword_Feedback(hitLocation, forceOfHit);
	}
}

void AWeaponBase::NET_Sword_Feedback_Implementation(FVector hitLocation, float forceOfHit)
{
	EDamageType damageType = UStaticLibrary::CalculateDamageType(forceOfHit);
	FeedBackSwordHit(hitLocation, damageType);

}
bool AWeaponBase::NET_Sword_Feedback_Validate(FVector hitLocation, float forceOfHit)
{
	return true;
}
void AWeaponBase::PlaySparkHit(UWorld* world, FVector spawnPoint)
{
	//UGameplayStatics::SpawnEmitterAtLocation(world, sparks, spawnPoint);
	//UGameplayStatics::PlaySoundAtLocation(world, swordSound, spawnPoint);
}

bool AWeaponBase::NET_OnCollisionResolved_Validate(ECollisionType collisionType, FVector impactPoint, FName bone)
{
	return true;
}

void AWeaponBase::OnKnightDestruction()
{
	if (Role == ROLE_Authority)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("KNIGHT DESTROYED"));
		Destroy();
	}
}



FName AWeaponBase::GetSwordType()
{
	return this->GetClass()->GetFName();

}