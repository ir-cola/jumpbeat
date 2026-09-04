#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StairSpark.generated.h"

/**
 * コンボ演出のヒバナ。
 *
 * 専用のパーティクル素材を持たないので、小さな立方体を
 * ばらまいて放物線で飛ばし、縮めながら消す方式で作る。
 * 段と同じ BasicShapeMaterial を使うので追加アセットは要らない。
 */
UCLASS()
class PUTICON_API AStairSpark : public AActor
{
	GENERATED_BODY()

public:
	AStairSpark();

	virtual void Tick(float DeltaSeconds) override;

	/** 弾けさせる。色と勢いを指定する */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void Burst(const FLinearColor& Color, float Power = 1.f);

	/**
	 * ★粒ごとに色相を散らして打ち上げる。コンボの花火用。
	 *   1色だと地味なので、虹色に散らして派手に見せる。
	 */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void BurstRainbow(float Power = 1.f);

protected:
	virtual void BeginPlay() override;

	/** 粒の数 */
	UPROPERTY(EditDefaultsOnly, Category = "Stair")
	int32 NumParticles = 12;

	/** 消えるまでの時間（秒） */
	UPROPERTY(EditDefaultsOnly, Category = "Stair")
	float Life = 0.85f;

	/** 粒の大きさ */
	UPROPERTY(EditDefaultsOnly, Category = "Stair")
	float ParticleSize = 0.16f;

	/** 落下の強さ */
	UPROPERTY(EditDefaultsOnly, Category = "Stair")
	float Gravity = 1500.f;

	UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> Meshes;
	UPROPERTY() TArray<TObjectPtr<class UMaterialInstanceDynamic>> Mats;

	TArray<FVector> Velocities;
	TArray<FVector> Starts;

	/** 粒ごとの色。虹色に散らすときに使う */
	TArray<FLinearColor> Colors;

	/** 虹色モードか */
	bool bRainbow = false;

	FLinearColor SparkColor = FLinearColor(1.f, 0.85f, 0.25f, 1.f);
	float Age = 0.f;
	bool bBurst = false;
};
