//Robert Sebenda, Manuel Cebreros
//Copyright 2016
#pragma once

#include "BaS_BaseActorComponent.h"
#include "StaticLibrary.h"
#include "BaS_PlayerController.h"
#include "KnightBase.h"
#include "BaS_ArmorBaseComponent.generated.h"


/**
*  armor durability types
*/


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BLOODANDSTEEL_API UBaS_ArmorBaseComponent : public UStaticMeshComponent//UBaS_BaseActorComponent
{
	GENERATED_BODY()

public:

	UBaS_ArmorBaseComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
	UMaterialInstanceDynamic* dynamMat;

	//determines texture map
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
	float BaseUVScale;

	//use for material change on damage
	UPROPERTY(BlueprintReadOnly, Category = "Material")
	float BaseDamageBlend;

	//set cap for scalar values
	UPROPERTY(BlueprintReadOnly, Category = "Material")
	float ScalarCap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destructable Mesh")
	UMaterial* damagedMaterial;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Destructable Mesh")
	UDestructibleComponent* BrokenMeshComponent;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Destructable Mesh")
	UDestructibleMesh* BrokenMesh;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadWrite, Category = "Armor")
	EArmorStatus armorState;

	//modifier on how much % of damage the armor piece takes (0.5 takes half damage) 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
	float armorDamagerModifer;

	//values where armor changes textures
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
	float BaseHealth;
		

	//threshold where armor becomes damaged
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
	float damagedHealthLevel;

	//current health of armor
	UPROPERTY(Replicated,VisibleAnywhere, BlueprintReadWrite, Category = "Armor")
	float armorHealth;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void AddToArmorLookup();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void OnHit(float damage, FVector hitLocation, AKnightBase* damageInstigator = nullptr);

	UFUNCTION(Reliable, Server, WithValidation, BlueprintCallable, Category = "Combat")
	void SERVER_OnHit(float damage, FVector hitLocation, AKnightBase* damageInstigator = nullptr);
	void SERVER_OnHit_Implementation(float damage, FVector hitLocation, AKnightBase* damageInstigator = nullptr);
	bool SERVER_OnHit_Validate(float damage, FVector hitLocation, AKnightBase* damageInstigator = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void WrapperFunction(float damage, FVector hitLocation, AKnightBase* damageInstigator);

	//UFUNCTION(BlueprintCallable, Category = "Combat")
	void UpdateArmorMesh();
	

	UFUNCTION(Reliable, NetMulticast, WithValidation, Category = "Combat")
	void ReplicateUpdateArmorMesh(const EArmorStatus &thisArmorState, const float &health);
	void ReplicateUpdateArmorMesh_Implementation(const EArmorStatus &thisArmorState, const float &health);
	bool ReplicateUpdateArmorMesh_Validate(const EArmorStatus &thisArmorState, const float &health);


	//FeedBack Section
	UFUNCTION(Reliable, NetMulticast, WithValidation, BlueprintCallable, Category = "Armor Feedback")
	void NET_Armor_Feedback(EDamageType typeofHit , const FVector hitLocation, const float &health, AKnightBase* damageInstigator = nullptr);
	void NET_Armor_Feedback_Implementation(EDamageType, const FVector hitLocation, const float &health, AKnightBase* damageInstigator = nullptr);
	bool NET_Armor_Feedback_Validate(EDamageType typeofHit, const FVector hitLocation, const float &health, AKnightBase* damageInstigator = nullptr);

	UPROPERTY()
		TArray<UParticleSystem*> sparks;

	UPROPERTY()
		TArray<USoundCue*> armorHitSounds;

	UPROPERTY()
		TArray<USoundCue*> armorBreakSounds;

		
	UFUNCTION(BlueprintImplementableEvent)
		void FeedBackArmorHit(const FVector &armorLocation, const float forceOfHit);

	UFUNCTION(BlueprintImplementableEvent)
		void PlayArmorBreak(const FVector &armorLocation, const float forceOfHit);

	//timer for flashing armor
	FTimerHandle armorFlashHandle;

	float armorFlashLength;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateArmorUI();
			
	UFUNCTION(BlueprintCallable, Category = "Feedback")
		void TurnOffArmorFlash();
	

	int noneEffect;
	int lightEffect;
	int mediumEffect;
	int HeavyEffect;
	int DevEffect;
};
