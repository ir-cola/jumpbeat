#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StairTypes.h"
#include "StairWidgets.generated.h"

class UTextBlock;
class UButton;
class UImage;
class UCanvasPanel;
class UBorder;
class UVerticalBox;
class UFont;
class AStairGameMode;
class AStairCharacter;
class UStairConfig;

/**
 * UI の共通基底。ウィジェットツリーは C++ 側で自前構築する。
 * Widget Blueprint を用意しなくても、このクラスをそのまま
 * CreateWidget に渡せば画面が出る。
 */
UCLASS(Abstract)
class PUTICON_API UStairWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintPure, Category = "Stair")
	AStairGameMode* GetStairGameMode() const;

	UFUNCTION(BlueprintPure, Category = "Stair")
	AStairCharacter* GetStairPlayer() const;

	UStairConfig* GetConfig() const;

protected:
	virtual void BuildUI(UCanvasPanel* Canvas) {}

	UTextBlock* MakeText(const FString& Name, const FString& Text, int32 Size,
		const FLinearColor& Color = FLinearColor::White,
		ETextJustify::Type Justify = ETextJustify::Center);

	UButton* MakeButton(const FString& Name, const FString& Label, int32 Size = 26);

	UBorder* MakeBox(const FString& Name, const FLinearColor& Color);

	void Place(UCanvasPanel* Canvas, class UWidget* W,
		const FVector2D& Anchor, const FVector2D& Alignment,
		const FVector2D& Position, const FVector2D& Size);

	/** ボタンを押した音。各ボタンのハンドラ先頭で呼ぶ */
	UFUNCTION(BlueprintCallable, Category = "Stair")
	void PlayButtonSound();

	/**
	 * ★画面の開閉（丸）。
	 *   単純な暗転ではなく、円が縮んで黒く閉じ、円が広がって開く。
	 *   円のテクスチャを持たないので、黒い矩形を格子状に敷き詰め、
	 *   中心からの距離で1マスずつ出し入れして円形を作る。
	 */
	void BuildIris(class UCanvasPanel* Canvas);

	/** 0=完全に開いている（透明）、1=完全に閉じている（真っ黒） */
	void SetIris(float Closed);

	/** 幕。全画面に1枚だけ置き、マテリアルで円をくり抜く */
	UPROPERTY() TObjectPtr<UImage> IrisImage;
	UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> IrisMat;

	/**
	 * 閉じてから何かをする。閉じ切ったら Action を呼ぶ。
	 * レベル遷移を「暗転しきってから」行うために使う。
	 */
	void CloseIrisThen(TFunction<void()> Action);

	/** 開閉の進行を毎フレーム進める。NativeTick から呼ぶ */
	void TickIris(float DeltaSeconds);

	/** -1=開いている最中 / 0=何もしない / 1=閉じている最中 */
	int32 IrisDir = -1;
	float IrisT = 1.f;          // 1=閉じきり、0=開ききり
	TFunction<void()> IrisAction;

	/** 遊び方の本文。タイトルとゲーム内で同じ文面を使う */
	static FString GetHowToText();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|UI")
	TObjectPtr<UFont> UIFont;
};

// =====================================================================

/** タイトル画面のどのパネルを見せているか */
UENUM()
enum class EStairTitlePanel : uint8
{
	Menu,        // ロゴ＋プレイ／チュートリアル
	Tutorial,    // 遊び方
	SongSelect   // 曲選択
};

/**
 * タイトル。
 * ★曲選択とチュートリアルはレベルを分けず、この画面に重ねる。
 *   背景で流れている階段をそのまま活かすため。
 */
UCLASS()
class PUTICON_API UStairTitleWidget : public UStairWidgetBase
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& Geo, float DeltaSeconds) override;

protected:
	virtual void BuildUI(UCanvasPanel* Canvas) override;

	UFUNCTION() void OnPlayClicked();
	UFUNCTION() void OnTutorialClicked();
	UFUNCTION() void OnBackClicked();

	UFUNCTION() void OnSong0Clicked();
	UFUNCTION() void OnSong1Clicked();
	UFUNCTION() void OnSong2Clicked();
	UFUNCTION() void OnSong3Clicked();
	UFUNCTION() void OnSong4Clicked();
	UFUNCTION() void OnSong5Clicked();

	/** 曲選択に並べる最大数 */
	static constexpr int32 MaxSongs = 6;

	void ShowPanel(EStairTitlePanel Panel);
	void SelectSong(int32 Index);

	UPROPERTY() TObjectPtr<UButton> PlayButton;
	UPROPERTY() TObjectPtr<UButton> TutorialButton;
	UPROPERTY() TObjectPtr<UButton> BackButton;
	UPROPERTY() TObjectPtr<UImage> LogoImage;

	UPROPERTY() TArray<TObjectPtr<UButton>> SongButtons;

	/** パネルごとの部品。まとめて表示切替する */
	UPROPERTY() TArray<TObjectPtr<UWidget>> MenuParts;
	UPROPERTY() TArray<TObjectPtr<UWidget>> TutorialParts;
	UPROPERTY() TArray<TObjectPtr<UWidget>> SongParts;

	/** ロゴ画像 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stair|UI")
	TObjectPtr<class UTexture2D> LogoTexture;

	EStairTitlePanel CurrentPanel = EStairTitlePanel::Menu;
};

// =====================================================================

/** BGM選択。曲を選んでゲームへ */
UCLASS()
class PUTICON_API UStairBGMSelectWidget : public UStairWidgetBase
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

protected:
	virtual void BuildUI(UCanvasPanel* Canvas) override;

	UFUNCTION() void OnSong0Clicked();
	UFUNCTION() void OnSong1Clicked();
	UFUNCTION() void OnSong2Clicked();
	UFUNCTION() void OnSong3Clicked();
	UFUNCTION() void OnBackClicked();

	void SelectSong(int32 Index);

	UPROPERTY() TArray<TObjectPtr<UButton>> SongButtons;
	UPROPERTY() TObjectPtr<UButton> BackButton;
	UPROPERTY() TObjectPtr<UTextBlock> WarnText;
};

// =====================================================================

/** プレイ中のHUD */
UCLASS()
class PUTICON_API UStairHUDWidget : public UStairWidgetBase
{
	GENERATED_BODY()

public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	virtual void BuildUI(UCanvasPanel* Canvas) override;

	/** 左上：緑のデジタル風タイマー */
	UPROPERTY() TObjectPtr<UTextBlock> TimerText;

	/** 右上：階段アイコン＋段数 */
	UPROPERTY() TObjectPtr<UTextBlock> StairIcon;
	UPROPERTY() TObjectPtr<UTextBlock> ScoreText;

	UPROPERTY() TObjectPtr<UTextBlock> CountdownText;
	UPROPERTY() TObjectPtr<UTextBlock> JudgeText;

	/** タイミング補正の表示。調整したときだけ出る */
	UPROPERTY() TObjectPtr<UTextBlock> TunerText;

	/** コンボ表示。画面右半分の中央に出す */
	UPROPERTY() TObjectPtr<UTextBlock> ComboText;
	UPROPERTY() TObjectPtr<UTextBlock> ComboLabel;

	/** ★押した位置の残像。判定ごとに色を変える */
	UPROPERTY() TArray<TObjectPtr<UBorder>> GhostMarks;

	/** 残りチャージの表示 */
	UPROPERTY() TArray<TObjectPtr<UBorder>> ChargePips;
	UPROPERTY() TObjectPtr<UTextBlock> ReloadText;
	UPROPERTY() TObjectPtr<UTextBlock> DirText;

	/** ★縦型の拍ゲージ。画面左中央。判定と同じパラメータから描く */
	UPROPERTY() TObjectPtr<UBorder> BeatTrack;
	UPROPERTY() TObjectPtr<UBorder> GreatZone;
	UPROPERTY() TObjectPtr<UBorder> PerfectZone;
	UPROPERTY() TObjectPtr<UBorder> MissStrip;
	UPROPERTY() TObjectPtr<UBorder> BeatCursor;

	UPROPERTY() TObjectPtr<UCanvasPanel> RootCanvas;

	/** ゲージの高さ（ピクセル）。縦いっぱいが1拍ぶん */
	UPROPERTY(EditDefaultsOnly, Category = "Stair|UI")
	float TrackHeight = 460.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stair|UI")
	float TrackWidth = 58.f;

	/** 画面左端からの距離 */
	UPROPERTY(EditDefaultsOnly, Category = "Stair|UI")
	float TrackLeftMargin = 120.f;

	/**
	 * PERFECT の中心を、ゲージの下端からの割合でどこに置くか。
	 * ★上端より少し下。ここより上へ行き過ぎると MISS。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Stair|UI")
	float PerfectCenterFrac = 0.84f;

	/** 最上部の MISS 帯（下端からの割合） */
	UPROPERTY(EditDefaultsOnly, Category = "Stair|UI")
	float MissStripFrac = 0.95f;

	/** 下端からの割合 → キャンバス上のY座標（中心基準・下が正） */
	float FracToY(float Frac) const { return TrackHeight * (0.5f - Frac); }
};

// =====================================================================

/** リザルト */
UCLASS()
class PUTICON_API UStairResultWidget : public UStairWidgetBase
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& Geo, float DeltaSeconds) override;

protected:
	virtual void BuildUI(UCanvasPanel* Canvas) override;

	UFUNCTION() void OnRetryClicked();
	UFUNCTION() void OnSelectClicked();
	UFUNCTION() void OnTitleClicked();

	UPROPERTY() TObjectPtr<UTextBlock> ScoreText;
	UPROPERTY() TObjectPtr<UTextBlock> ReasonText;
	UPROPERTY() TObjectPtr<UTextBlock> DetailText;
	UPROPERTY() TObjectPtr<UButton> RetryButton;
	UPROPERTY() TObjectPtr<UButton> SelectButton;
	UPROPERTY() TObjectPtr<UButton> TitleButton;
};
