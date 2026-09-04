#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StairBullet.generated.h"

/**
 * 弾。小さな黒い球体。
 *
 * ★狙いを付ける操作は無く、撃つと自動的に対象の隕石へ向かう。
 *   プレイヤーがやるのは「いつ撃つか」だけ。
 *   対象が無い（＝射程内に隕石がいない）場合は、まっすぐ飛んで消える。
 */
UCLASS()
class PUTICON_API AStairBullet : public AActor
{
	GENERATED_BODY()

public:
	AStairBullet();

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * 発射する。
	 * @param InTarget 追いかける隕石。nullptr なら直進して消える
	 * @param InSpeed  速さ（cm/秒）
	 */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void Fire(class AStairMeteor* InTarget, const FVector& Forward, float InSpeed);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stair")
	TObjectPtr<class UStaticMeshComponent> Mesh;

	UPROPERTY()
	TWeakObjectPtr<class AStairMeteor> Target;

	UPROPERTY(EditDefaultsOnly, Category = "Stair")
	FLinearColor BulletColor = FLinearColor(0.02f, 0.02f, 0.03f, 1.f);

	/** 当たったとみなす距離 */
	UPROPERTY(EditDefaultsOnly, Category = "Stair")
	float HitDistance = 90.f;

	FVector Velocity = FVector::ZeroVector;
	float Speed = 4200.f;
	float Age = 0.f;

	/** 対象が消えた場合などに残り続けないよう、寿命を持たせる */
	UPROPERTY(EditDefaultsOnly, Category = "Stair")
	float MaxLife = 1.2f;

public:
	void SetBulletScale(float S) { BulletScale = FMath::Max(0.02f, S); }

protected:
	float BulletScale = 0.22f;
};
