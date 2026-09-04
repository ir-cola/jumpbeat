#pragma once

#include "CoreMinimal.h"
#include "ChargeMinigameBase.h"
#include "ChargeMinigame_ClickTargets.generated.h"

/**
 * 飛来球1個ぶんの情報。BP側がこれを見て描画する。
 */
USTRUCT(BlueprintType)
struct FDanFlyingTarget
{
	GENERATED_BODY()

	/** 出現時刻（ミニゲーム開始からの秒） */
	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	float SpawnTime = 0.f;

	/** 画面を横切りきるまでの秒数 */
	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	float TravelTime = 1.2f;

	/** 開始位置（スクリーン座標・ピクセル） */
	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	FVector2D StartPos = FVector2D::ZeroVector;

	/** 終了位置 */
	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	FVector2D EndPos = FVector2D::ZeroVector;

	/** true なら赤球（クリックすると暴発） */
	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	bool bIsRed = false;

	/** 既にクリックされたか */
	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	bool bTaken = false;
};

/**
 * チャージ・ミニゲーム「飛来球クリック」。設計書 3-4。
 *
 *   ClickDuration 秒のあいだに青球と赤球が飛来する。
 *   c = 青球の捕獲数 / ClickBlueCount
 *   赤球をクリックすると即暴発。
 *
 * 速さではなく識別と精度を問う。他3種と明確に毛色が違う。
 */
UCLASS(Blueprintable, ClassGroup = (Dan), meta = (BlueprintSpawnableComponent))
class PUTICON_API UChargeMinigame_ClickTargets : public UChargeMinigameBase
{
	GENERATED_BODY()

public:
	virtual FText GetDisplayName() const override;
	virtual EChargeMinigameType GetMinigameType() const override;
	virtual float GetProgress() const override;

	virtual void BeginMinigame() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	virtual void OnClickAt(FVector2D ScreenPos) override;

	/** 全ターゲット。BP側が毎フレーム読んで描画する */
	UFUNCTION(BlueprintPure, Category = "Dan")
	const TArray<FDanFlyingTarget>& GetTargets() const { return Targets; }

	/** ミニゲーム開始からの経過秒 */
	UFUNCTION(BlueprintPure, Category = "Dan")
	float GetElapsed() const { return Elapsed; }

	/** 指定インデックスの球の現在位置。画面外なら bVisible=false */
	UFUNCTION(BlueprintPure, Category = "Dan")
	FVector2D GetTargetPosition(int32 Index, bool& bVisible) const;

	UFUNCTION(BlueprintPure, Category = "Dan")
	int32 GetCaughtCount() const { return CaughtCount; }

	/** 球を取った瞬間。BP側でエフェクトを出す */
	UFUNCTION(BlueprintImplementableEvent, Category = "Dan")
	void OnTargetCaught(int32 Index, bool bWasRed);

protected:
	void BuildTargets();

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	TArray<FDanFlyingTarget> Targets;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	float Elapsed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	int32 CaughtCount = 0;

	/** 画面サイズ。BeginMinigame で取得する */
	FVector2D ViewportSize = FVector2D(1920.f, 1080.f);
};
