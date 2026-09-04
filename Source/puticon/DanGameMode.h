#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DanTypes.h"
#include "DanGameMode.generated.h"

class UDanGameParams;
class UChargeMinigameBase;
class UDefenseModeBase;
class ADanEnergyBall;
class UUserWidget;

/**
 * ゲーム全体の進行役。設計書 6-1 のステートマシンを持つ。
 * この10状態がゲームの全てで、これ以外の状態は存在しない。
 */
UCLASS()
class PUTICON_API ADanGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADanGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// ---------------- 状態 ----------------

	UFUNCTION(BlueprintPure, Category = "Dan")
	EDanTrialState GetState() const { return State; }

	UFUNCTION(BlueprintCallable, Category = "Dan")
	void GotoState(EDanTrialState NewState);

	UFUNCTION(BlueprintPure, Category = "Dan")
	float GetStateTime() const { return StateTime; }

	/** 状態が切り替わった瞬間。BP側で演出・UI切替を行う */
	UFUNCTION(BlueprintImplementableEvent, Category = "Dan")
	void OnStateEntered(EDanTrialState NewState);

	/** 選択フェーズの残り秒。UIのタイマー表示用 */
	UFUNCTION(BlueprintPure, Category = "Dan")
	float GetSelectRemaining() const;

	// ---------------- 入力の受け口（PlayerControllerから呼ばれる）----------------

	UFUNCTION(BlueprintCallable, Category = "Dan")
	void HandleActionPressed();

	UFUNCTION(BlueprintCallable, Category = "Dan")
	void HandleActionReleased();

	UFUNCTION(BlueprintCallable, Category = "Dan")
	void HandlePointerMoved(FVector2D Delta);

	UFUNCTION(BlueprintCallable, Category = "Dan")
	void HandleClickAt(FVector2D ScreenPos);

	// ---------------- 防衛の選択（UIから呼ぶ）----------------

	/** 選択フェーズ中に呼ぶと防衛方法が決まり、即座に帰還フェーズへ進む */
	UFUNCTION(BlueprintCallable, Category = "Dan")
	void SelectDefense(EDefenseType Type);

	UFUNCTION(BlueprintPure, Category = "Dan")
	EDefenseType GetSelectedDefense() const { return SelectedDefense; }

	// ---------------- 進行データ ----------------

	UFUNCTION(BlueprintPure, Category = "Dan")
	int32 GetTrialIndex() const { return TrialIndex; }

	UFUNCTION(BlueprintPure, Category = "Dan")
	int32 GetTotalScore() const { return TotalScore; }

	UFUNCTION(BlueprintPure, Category = "Dan")
	const TArray<FDanTrialRecord>& GetRecords() const { return Records; }

	UFUNCTION(BlueprintPure, Category = "Dan")
	float GetCurrentCharge() const { return CurrentCharge; }

	/** 「かっこよさ度」の表示文字列。7桁ゼロ埋め＋% */
	UFUNCTION(BlueprintPure, Category = "Dan")
	FString GetScoreText() const;

	/** 棒立ちを含むプレイなら true。この場合スコアは「測定不能」 */
	UFUNCTION(BlueprintPure, Category = "Dan")
	bool IsUnmeasurable() const;

	/** ランキングを更新したか。リザルトで「記録更新」を出すのに使う */
	UFUNCTION(BlueprintPure, Category = "Dan")
	bool WasRankUpdated() const { return bRankUpdated; }

	UFUNCTION(BlueprintPure, Category = "Dan")
	UDanGameParams* GetParams() const { return Params; }

	/** いま使っているチャージ・ミニゲーム */
	UFUNCTION(BlueprintPure, Category = "Dan")
	UChargeMinigameBase* GetChargeMinigame() const;

	/** いま使っている防衛モード */
	UFUNCTION(BlueprintPure, Category = "Dan")
	UDefenseModeBase* GetDefenseMode() const;

	UFUNCTION(BlueprintPure, Category = "Dan")
	ADanEnergyBall* GetEnergyBall() const { return EnergyBall; }

	/** もう一度プレイする */
	UFUNCTION(BlueprintCallable, Category = "Dan")
	void RestartGame();

protected:
	void UpdateState(float DeltaSeconds);
	void CommitTrialScore();
	void BeginTrial();
	void DrawDebug() const;

	/** 状態に応じて選択UI・リザルトUIを出し入れする */
	void UpdateWidgets();

	// ---------------- UI ----------------

	/** BP_DanGameMode の詳細で WBP_HUD を指定する */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dan|UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dan|UI")
	TSubclassOf<UUserWidget> SelectWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dan|UI")
	TSubclassOf<UUserWidget> ResultWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Dan|UI")
	TObjectPtr<UUserWidget> HUDWidget;

	UPROPERTY(BlueprintReadOnly, Category = "Dan|UI")
	TObjectPtr<UUserWidget> SelectWidget;

	UPROPERTY(BlueprintReadOnly, Category = "Dan|UI")
	TObjectPtr<UUserWidget> ResultWidget;

	// ---------------- プロパティ ----------------

	/** ★調整用パラメータ。BP_DanGameMode の詳細で DA_GameParams を指定する */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dan")
	TObjectPtr<UDanGameParams> Params;

	/** 生成するエネルギー弾のクラス。BP_EnergyBall を指定する */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dan")
	TSubclassOf<ADanEnergyBall> EnergyBallClass;

	/** 選択フェーズでゲームを遅くする倍率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dan")
	float SelectTimeDilation = 0.25f;

	/** 画面に状態名を出す。Phase 1 の確認用 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dan|Debug")
	bool bShowDebug = true;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	EDanTrialState State = EDanTrialState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	float StateTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	int32 TrialIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	int32 TotalScore = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	TArray<FDanTrialRecord> Records;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	float CurrentCharge = 0.f;

	/** ジャストの瞬間（ワールド時刻・秒）*/
	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	float PerfectTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	EChargeMinigameType ActiveMinigame = EChargeMinigameType::Hold;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	EDefenseType SelectedDefense = EDefenseType::Slash;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	TObjectPtr<ADanEnergyBall> EnergyBall;

	UPROPERTY(BlueprintReadOnly, Category = "Dan")
	bool bRankUpdated = false;
};
