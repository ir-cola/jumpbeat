#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StairTypes.h"
#include "StairMeteor.generated.h"

/**
 * 降ってくる障害物（隕石）。
 *
 * 本体は黒寄りの灰色のキューブで、回転しながら斜めに落ちてくる。
 * 煙と炎の尾を引く。
 *
 * ★撃つタイミングは隕石自身が持つ。
 *   縮んでくる白い円が、固定の黄緑リングに重なった瞬間が当たり。
 *   画面端のゲージを見に行かずに済み、複数同時に降っても
 *   それぞれの残り時間が一目で分かる。
 */
UCLASS()
class PUTICON_API AStairMeteor : public AActor
{
	GENERATED_BODY()

public:
	AStairMeteor();

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * 落下を開始する。
	 * @param ImpactLocation 着弾する位置
	 * @param LeadSeconds    着弾までの時間（＝撃つ猶予）
	 */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void Launch(const FVector& ImpactLocation, float LeadSeconds);

	/** 撃ち落とされた。爆散して消える */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void Destroyed_ByShot();

	/**
	 * いまの「縮み具合」。0=出現直後、1=着弾。
	 * 白い円の半径はこれで決まる。
	 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	float GetApproach() const { return Approach; }

	/**
	 * 撃つ判定。黄緑リングにどれだけ近いかで決まる。
	 * ★ゲージ側ではなく隕石側が判定を持つ。
	 */
	UFUNCTION(BlueprintPure, Category = "Stair")
	EStairJudge JudgeShot() const;

	/** まだ落下中か */
	UFUNCTION(BlueprintPure, Category = "Stair")
	bool IsAlive() const { return bAlive; }

	/** 着弾したか（撃ち漏らした） */
	UFUNCTION(BlueprintPure, Category = "Stair")
	bool HasImpacted() const { return bImpacted; }

	UPROPERTY(BlueprintReadOnly, Category = "Stair")
	FVector Impact = FVector::ZeroVector;

protected:
	virtual void BeginPlay() override;

	/** 尾を引く煙と炎を更新する */
	void UpdateTrail(float DeltaSeconds);

	/** 本体。灰色のキューブ */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stair")
	TObjectPtr<class UStaticMeshComponent> Body;

	/**
	 * 撃つタイミングを見せる輪。カメラに正対させる。
	 * ★円のテクスチャを持たないので、小さなキューブを円周に並べて環を作る。
	 *   ゲーム全体がキューブで出来ているので見た目も揃う。
	 */
	UPROPERTY() TObjectPtr<class USceneComponent> RingRoot;

	/** 赤い円盤（判定の外＝MISS） */
	UPROPERTY() TObjectPtr<class UStaticMeshComponent> Disc;

	/** 黄緑のリング（当たりの帯）。固定 */
	UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> HitRing;

	/** 白い円。時間とともに縮んでくる */
	UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> AppRing;

	/** 尾を引く粒 */
	UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> Trail;
	UPROPERTY() TArray<TObjectPtr<class UMaterialInstanceDynamic>> TrailMats;

	UPROPERTY()
	TObjectPtr<class UMaterialInstanceDynamic> BodyMat;

	/** 通った位置の記録。尾を並べるのに使う */
	TArray<FVector> History;

	/** 黄緑リングの半径 */
	UPROPERTY(EditDefaultsOnly, Category = "Stair")
	float RingRadius = 95.f;

	/** 本体と輪の大きさ */
	float MeteorScale = 0.85f;

	// ---- 設定 ----

	UPROPERTY(EditDefaultsOnly, Category = "Stair")
	FLinearColor BodyColor = FLinearColor(0.10f, 0.10f, 0.12f, 1.f);

	/** 尾の粒をどれくらいの間隔で出すか（秒） */
	UPROPERTY(EditDefaultsOnly, Category = "Stair")
	float TrailInterval = 0.02f;

	/** 尾の粒が消えるまで */
	UPROPERTY(EditDefaultsOnly, Category = "Stair")
	float TrailLife = 0.55f;

	FVector Start = FVector::ZeroVector;
	FRotator Spin = FRotator(220.f, 160.f, 190.f);

	float Lead = 2.2f;
	float Elapsed = 0.f;
	float Approach = 0.f;
	float TrailTimer = 0.f;

	bool bAlive = true;
	bool bImpacted = false;

	/** 判定の幅。GameMode から Config の値が入る */
	float PerfectWidth = 0.10f;
	float GreatWidth = 0.26f;

public:
	/** 判定の幅を設定する */
	void SetJudgeWidths(float InPerfect, float InGreat)
	{
		PerfectWidth = InPerfect;
		GreatWidth = InGreat;
	}

	void SetSpin(const FRotator& InSpin) { Spin = InSpin; }
	void SetMeteorScale(float S) { MeteorScale = FMath::Max(0.05f, S); }
};
