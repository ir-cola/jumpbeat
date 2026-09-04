#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DanTypes.h"
#include "DefenseModeBase.generated.h"

class UDanGameParams;

/**
 * 防衛モードの基底。設計書 4章・6章。
 *
 * ★断と弾は得点が完全に同じ。違うのは「どの瞬間を t_input とみなすか」だけ。
 *     断 = 押した瞬間      (OnInputPressed)
 *     弾 = 離した瞬間      (OnInputReleased)
 *   よってスコア計算側は防衛モードを一切参照しなくてよい。
 */
UCLASS(Abstract, Blueprintable, ClassGroup = (Dan), meta = (BlueprintSpawnableComponent))
class PUTICON_API UDefenseModeBase : public UActorComponent
{
	GENERATED_BODY()

public:
	UDefenseModeBase();

	/** 防衛フェーズ開始。チャージ量と「ジャストの瞬間」を受け取る */
	UFUNCTION(BlueprintCallable, Category = "Dan")
	virtual void BeginDefense(float InCharge, float InPerfectTime);

	// ---- 入力 ----
	virtual void OnInputPressed(float /*Now*/) {}
	virtual void OnInputReleased(float /*Now*/) {}

	/** 着弾時刻に GameMode から呼ばれる。未入力なら Failed を確定させる */
	UFUNCTION(BlueprintCallable, Category = "Dan")
	virtual void ResolveAtImpact(float Now);

	UFUNCTION(BlueprintPure, Category = "Dan")
	EDefenseResult GetResult() const { return Result; }

	UFUNCTION(BlueprintPure, Category = "Dan")
	float GetAccuracy() const { return Accuracy; }

	UFUNCTION(BlueprintPure, Category = "Dan")
	bool IsResolved() const { return Result != EDefenseResult::Pending; }

	/** 表示用の名前（「断」「弾」など） */
	UFUNCTION(BlueprintPure, Category = "Dan")
	virtual FText GetDisplayName() const;

	/** 防衛方法の種類。GameMode がこれを見て使うコンポーネントを選ぶ */
	UFUNCTION(BlueprintPure, Category = "Dan")
	virtual EDefenseType GetDefenseType() const;

	/** 判定が確定した瞬間。BP側で演出を出す */
	UFUNCTION(BlueprintImplementableEvent, Category = "Dan")
	void OnResolved(EDefenseResult InResult, float InAccuracy);

	UFUNCTION(BlueprintCallable, Category = "Dan")
	void SetParams(UDanGameParams* InParams) { Params = InParams; }

protected:
	/**
	 * 共通のタイミング判定。断も弾もこれを呼ぶだけ。
	 * 設計書5章:  a = clamp(1 - |t_input - t_perfect| / w(c), 0, 1)
	 */
	void EvaluateTiming(float InputTime);

	/** 結果を確定させて OnResolved を呼ぶ */
	void Finish(EDefenseResult InResult, float InAccuracy);

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	TObjectPtr<UDanGameParams> Params;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	float Charge = 0.f;

	/** ジャストの瞬間（ワールド時刻・秒） */
	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	float PerfectTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	float Accuracy = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	EDefenseResult Result = EDefenseResult::Pending;
};
