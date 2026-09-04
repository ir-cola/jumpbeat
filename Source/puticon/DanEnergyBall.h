#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DanEnergyBall.generated.h"

class USceneComponent;

/** 弾の飛行フェーズ */
UENUM(BlueprintType)
enum class EBallPhase : uint8
{
	Idle      UMETA(DisplayName = "待機"),
	FlyingOut UMETA(DisplayName = "飛去中"),
	Orbiting  UMETA(DisplayName = "周回中"),
	Returning UMETA(DisplayName = "帰還中"),
	Done      UMETA(DisplayName = "終了")
};

/**
 * エネルギー弾。設計書 2章・8章。
 *
 * 発射 → 地球を回り込む → 正面奥から帰還、という経路を辿る。
 * 見た目（Niagara・発光マテリアル）は BP_EnergyBall 側で付ける。
 *
 * ★帰還の予兆は着弾1.5秒前から始まる。これは十段で「弾」を選んだときの
 *   長押し開始タイミングと一致させてある（設計書 6-1）。
 */
UCLASS()
class PUTICON_API ADanEnergyBall : public AActor
{
	GENERATED_BODY()

public:
	ADanEnergyBall();

	virtual void Tick(float DeltaSeconds) override;

	/** 発射する。FlyDuration 秒かけて飛び去る */
	UFUNCTION(BlueprintCallable, Category = "Dan")
	void Fire(const FVector& InOrigin, float InCharge, float FlyDuration);

	/**
	 * 帰還を開始する。InReturnTime 秒後にちょうど Origin へ戻る。
	 * GameMode が Returning 状態に入るときに呼ぶ。
	 */
	UFUNCTION(BlueprintCallable, Category = "Dan")
	void BeginReturn(float InReturnTime);

	/** 弾を消す */
	UFUNCTION(BlueprintCallable, Category = "Dan")
	void Vanish();

	UFUNCTION(BlueprintPure, Category = "Dan")
	EBallPhase GetPhase() const { return Phase; }

	/** 着弾までの残り秒。予兆演出の判定に使う */
	UFUNCTION(BlueprintPure, Category = "Dan")
	float GetTimeToImpact() const;

	/** 0〜1。帰還の進捗 */
	UFUNCTION(BlueprintPure, Category = "Dan")
	float GetReturnAlpha() const;

	// ---- 演出フック（BP側で実装）----

	UFUNCTION(BlueprintImplementableEvent, Category = "Dan")
	void OnFired(float InCharge);

	/** 帰還開始。ここから接近音を鳴らし始める */
	UFUNCTION(BlueprintImplementableEvent, Category = "Dan")
	void OnReturnBegan(float InReturnTime);

	/** 着弾1.5秒前。予兆演出の合図 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Dan")
	void OnWarning();

	UFUNCTION(BlueprintImplementableEvent, Category = "Dan")
	void OnVanished();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dan")
	TObjectPtr<USceneComponent> Root;

	/** 予兆を出すタイミング（着弾までの秒数） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dan")
	float WarningLeadTime = 1.5f;

	/** 飛び去る方向と距離 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dan")
	float FlyDistance = 6000.f;

	/** 周回中に回り込む横方向の量 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dan")
	float OrbitSideOffset = 4000.f;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	EBallPhase Phase = EBallPhase::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	float Charge = 0.f;

	/** 発射地点＝着弾地点 */
	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	FVector Origin = FVector::ZeroVector;

	/** 飛び去りきった位置 */
	FVector FarPoint = FVector::ZeroVector;

	float PhaseTime = 0.f;
	float FlyTime = 1.5f;
	float ReturnTime = 3.f;
	bool bWarned = false;
};
