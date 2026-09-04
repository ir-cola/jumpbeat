#pragma once

#include "CoreMinimal.h"
#include "ChargeMinigameBase.h"
#include "ChargeMinigame_Hold.generated.h"

/**
 * チャージ・ミニゲーム「溜め（長押し）」。設計書 3-1。
 *
 *   c    = clamp(押していた時間 / HoldTimeMax, 0, 1)
 *   暴発 = 押していた時間 > HoldTimeBurst
 *
 * 十段(c>=0.9)を狙うには 3.00〜3.30秒 の 0.3秒の窓で離す必要がある。
 * この暴発があるおかげで「押しっぱなしで最大まで溜める」が最適解にならない。
 */
UCLASS(Blueprintable, ClassGroup = (Dan), meta = (BlueprintSpawnableComponent))
class PUTICON_API UChargeMinigame_Hold : public UChargeMinigameBase
{
	GENERATED_BODY()

public:
	virtual FText GetDisplayName() const override;
	virtual EChargeMinigameType GetMinigameType() const override;
	virtual float GetProgress() const override;

	virtual void BeginMinigame() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	virtual void OnPressStart(float Now) override;
	virtual void OnPressEnd(float Now) override;

	/** 押している時間(秒)。UIのゲージ表示に使う */
	UFUNCTION(BlueprintPure, Category = "Dan")
	float GetHeldTime() const { return HeldTime; }

	UFUNCTION(BlueprintPure, Category = "Dan")
	bool IsHolding() const { return bHolding; }

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	bool bHolding = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	float HeldTime = 0.f;
};
