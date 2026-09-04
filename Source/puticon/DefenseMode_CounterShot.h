#pragma once

#include "CoreMinimal.h"
#include "DefenseModeBase.h"
#include "DefenseMode_CounterShot.generated.h"

/**
 * 防衛「弾」＝エネルギー弾を撃ち返す。設計書 4-3。
 *
 * 断と得点は完全に同じ。違うのは入力の形だけ。
 *   ・判定に使うのは「離した瞬間」
 *   ・こちらも溜める必要があるので、最低 GetHoldRequired(c) 秒の長押しが要る
 *       足りない → 不発(Misfire)。タイミングが完璧でも被弾する
 *   ・長押しが HoldMargin 秒を超えて長すぎる → 過充填(Overfilled)で手元暴発
 *       ※ HoldMargin = 0 なら過充填判定は無効
 *
 * 十段なら着弾の1.5秒前には押し始めていなければならない。
 * 断が「反応」の勝負なら、弾は「先読み」の勝負。
 */
UCLASS(Blueprintable, ClassGroup = (Dan), meta = (BlueprintSpawnableComponent))
class PUTICON_API UDefenseMode_CounterShot : public UDefenseModeBase
{
	GENERATED_BODY()

public:
	virtual FText GetDisplayName() const override;
	virtual EDefenseType GetDefenseType() const override;

	virtual void BeginDefense(float InCharge, float InPerfectTime) override;
	virtual void OnInputPressed(float Now) override;
	virtual void OnInputReleased(float Now) override;

	UFUNCTION(BlueprintPure, Category = "Dan")
	bool IsHolding() const { return bHolding; }

	/** 押し始めてからの経過秒。UIのゲージ表示に使う */
	UFUNCTION(BlueprintPure, Category = "Dan")
	float GetHeldTime(float Now) const { return bHolding ? (Now - PressTime) : 0.f; }

	/** このチャージ量で必要な長押し時間(秒) */
	UFUNCTION(BlueprintPure, Category = "Dan")
	float GetRequiredHold() const;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	bool bHolding = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	float PressTime = 0.f;
};
