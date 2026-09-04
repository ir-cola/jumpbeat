#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DanTypes.h"
#include "ChargeMinigameBase.generated.h"

class UDanGameParams;

/**
 * チャージ・ミニゲームの基底。設計書 3章・6章。
 *
 * ★契約: どんな遊びであれ GetChargeAmount() が 0〜1 を返すことだけを守る。
 *   これさえ守れば、ミニゲームを何個増やしてもスコア計算にも防衛フェーズにも
 *   一切手を入れる必要がない。
 */
UCLASS(Abstract, Blueprintable, ClassGroup = (Dan), meta = (BlueprintSpawnableComponent))
class PUTICON_API UChargeMinigameBase : public UActorComponent
{
	GENERATED_BODY()

public:
	UChargeMinigameBase();

	/** ミニゲーム開始。GameMode が Charging に入るときに呼ぶ */
	UFUNCTION(BlueprintCallable, Category = "Dan")
	virtual void BeginMinigame();

	/** ミニゲーム終了 */
	UFUNCTION(BlueprintCallable, Category = "Dan")
	virtual void EndMinigame();

	/** ★このミニゲームの唯一の出力。0〜1 */
	UFUNCTION(BlueprintPure, Category = "Dan")
	float GetChargeAmount() const { return Charge; }

	/** 暴発したか */
	UFUNCTION(BlueprintPure, Category = "Dan")
	bool IsBurst() const { return bBurst; }

	/** ミニゲームが終わったか。GameMode はこれを見て次の状態へ進む */
	UFUNCTION(BlueprintPure, Category = "Dan")
	bool IsFinished() const { return bFinished; }

	/** 表示用の名前（「溜め」「連打」など） */
	UFUNCTION(BlueprintPure, Category = "Dan")
	virtual FText GetDisplayName() const;

	/** ミニゲームの種類。GameMode がこれを見て使うコンポーネントを選ぶ */
	UFUNCTION(BlueprintPure, Category = "Dan")
	virtual EChargeMinigameType GetMinigameType() const;

	/** 進捗 0〜1。UIのタイマー表示に使う（時間制限が無いものは0） */
	UFUNCTION(BlueprintPure, Category = "Dan")
	virtual float GetProgress() const { return 0.f; }

	// ---- 入力。派生クラスがそれぞれ解釈する ----
	virtual void OnPressStart(float /*Now*/) {}
	virtual void OnPressEnd(float /*Now*/) {}
	virtual void OnPointerMove(FVector2D /*Delta*/) {}
	virtual void OnClickAt(FVector2D /*ScreenPos*/) {}

	// ---- 演出フック。BP側で実装する（C++には中身を書かない）----

	/** 段位が上がった瞬間。ここでエフェクトと音を出す */
	UFUNCTION(BlueprintImplementableEvent, Category = "Dan")
	void OnChargeStepUp(int32 NewDan);

	/** 暴発した瞬間 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Dan")
	void OnBurst();

	/** パラメータを注入する。GameMode から呼ぶ */
	UFUNCTION(BlueprintCallable, Category = "Dan")
	void SetParams(UDanGameParams* InParams) { Params = InParams; }

protected:
	/** 段位が変わったら OnChargeStepUp を呼ぶ。派生の Tick から使う */
	void UpdateDanAndNotify();

	/** 暴発させて終了する。全ミニゲーム共通 */
	void TriggerBurst();

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	TObjectPtr<UDanGameParams> Params;

	/** 0〜1 */
	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	float Charge = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	bool bBurst = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	bool bFinished = false;

	/** 直近に通知した段位。段位アップ検出用 */
	int32 LastNotifiedDan = 0;
};
