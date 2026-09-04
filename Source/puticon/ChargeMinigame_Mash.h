#pragma once

#include "CoreMinimal.h"
#include "ChargeMinigameBase.h"
#include "ChargeMinigame_Mash.generated.h"

/**
 * チャージ・ミニゲーム「連打」。設計書 3-2。
 *
 *   1打ごとに +MashPerTap、毎秒 -MashDecay で自然減衰。
 *   MashDuration 秒経過した時点の値が c。
 *   ゲージが MashBurst を超えた瞬間に過熱暴発。
 *
 * 減衰があるので「序盤に稼いで後半で調整」という戦略が生まれ、
 * 最後まで全力連打すると暴発する。
 */
UCLASS(Blueprintable, ClassGroup = (Dan), meta = (BlueprintSpawnableComponent))
class PUTICON_API UChargeMinigame_Mash : public UChargeMinigameBase
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

	/** 連打した回数。UI表示用 */
	UFUNCTION(BlueprintPure, Category = "Dan")
	int32 GetTapCount() const { return TapCount; }

	/** 残り時間(秒) */
	UFUNCTION(BlueprintPure, Category = "Dan")
	float GetRemainingTime() const;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	float Elapsed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	int32 TapCount = 0;
};
