#pragma once

#include "CoreMinimal.h"
#include "ChargeMinigameBase.h"
#include "ChargeMinigame_Rotate.generated.h"

/**
 * チャージ・ミニゲーム「ぐるぐる回転」。設計書 3-3。
 *
 *   マウスで円を描き続けると累積回転角が増える。
 *   c = 累積周回数 / RotateTarget
 *   逆回転すると減り、手を止めても毎秒 RotateDecay ずつ減る。
 *   RotateBurst 周を超えると暴発。
 *
 * 実装方針: OnPointerMove は移動量しか来ないので、
 * 内部で仮想的なカーソル位置を持ち、その偏角の変化を積算する。
 */
UCLASS(Blueprintable, ClassGroup = (Dan), meta = (BlueprintSpawnableComponent))
class PUTICON_API UChargeMinigame_Rotate : public UChargeMinigameBase
{
	GENERATED_BODY()

public:
	virtual FText GetDisplayName() const override;
	virtual EChargeMinigameType GetMinigameType() const override;
	virtual float GetProgress() const override;

	virtual void BeginMinigame() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	virtual void OnPointerMove(FVector2D Delta) override;

	/** 累積周回数 */
	UFUNCTION(BlueprintPure, Category = "Dan")
	float GetTurns() const { return Turns; }

	/** 仮想カーソルの角度(度)。BP側で渦エフェクトの向きに使える */
	UFUNCTION(BlueprintPure, Category = "Dan")
	float GetCurrentAngleDeg() const { return LastAngleDeg; }

protected:
	/** 中心からの仮想カーソル位置 */
	FVector2D VirtualPos = FVector2D(100.f, 0.f);

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	float Turns = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	float LastAngleDeg = 0.f;

	/** 最後に動かしてからの経過秒 */
	float IdleTime = 0.f;

	bool bStarted = false;
};
