#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DanTypes.h"
#include "DanWidgets.generated.h"

class UTextBlock;
class UButton;
class UWidgetSwitcher;
class UProgressBar;
class ADanGameMode;

/**
 * UI の共通基底。
 * ロジックは全部C++に持たせ、WBP側は見た目だけを持つ。
 * 子ウィジェットは BindWidgetOptional なので、無くてもクラッシュしない。
 */
UCLASS(Abstract)
class PUTICON_API UDanWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Dan")
	ADanGameMode* GetDanGameMode() const;
};

// =====================================================================

/** プレイ中のHUD。状態・段位・チャージ量・スコアを出す */
UCLASS()
class PUTICON_API UDanHUDWidget : public UDanWidgetBase
{
	GENERATED_BODY()

public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	/** 「溜め」「連打」などミニゲーム名 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MinigameText;

	/** 「五段」 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DanText;

	/** チャージ量のゲージ */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ChargeBar;

	/** 「かっこよさ度 0012300 %」 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ScoreText;

	/** 状態に応じた案内文 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GuideText;

	/** トライ数「1 / 3」 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TrialText;
};

// =====================================================================

/** 防衛の選択。断／弾の二択と残り時間 */
UCLASS()
class PUTICON_API UDanSelectWidget : public UDanWidgetBase
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	UFUNCTION()
	void OnSlashClicked();

	UFUNCTION()
	void OnCounterClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SlashButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CounterButton;

	/** 残り秒 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TimerText;

	/** 「八段。どう迎え撃つ？」 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PromptText;
};

// =====================================================================

/** リザルト。かっこよさ度と内訳 */
UCLASS()
class PUTICON_API UDanResultWidget : public UDanWidgetBase
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** GameMode から呼ばれて内容を更新する */
	UFUNCTION(BlueprintCallable, Category = "Dan")
	void Refresh();

protected:
	UFUNCTION()
	void OnRetryClicked();

	UFUNCTION()
	void OnTitleClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ScoreText;

	/** トライごとの内訳 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailText;

	/** 「記録更新」 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RankText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RetryButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> TitleButton;
};

// =====================================================================

/** タイトル。4ボタン＋ランキング／遊び方パネル */
UCLASS()
class PUTICON_API UDanTitleWidget : public UDanWidgetBase
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

protected:
	UFUNCTION()
	void OnStartClicked();

	UFUNCTION()
	void OnRankingClicked();

	UFUNCTION()
	void OnHowToClicked();

	UFUNCTION()
	void OnQuitClicked();

	UFUNCTION()
	void OnBackClicked();

	/** ランキングの中身を作って RankingText に流し込む */
	void BuildRankingText();

	/** 0=メニュー 1=ランキング 2=遊び方 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> Switcher;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> StartButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RankingButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> HowToButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RankingText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HowToText;

	/** 遷移先のゲームレベル名 */
	UPROPERTY(EditDefaultsOnly, Category = "Dan")
	FName GameLevelName = TEXT("L_Game");
};
