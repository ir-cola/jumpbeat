#include "StairWidgets.h"
#include "StairGameMode.h"
#include "StairCharacter.h"
#include "StairConfig.h"
#include "StairMusicClock.h"
#include "StairGameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"

// =====================================================================
// 共通基底
// =====================================================================

TSharedRef<SWidget> UStairWidgetBase::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = Canvas;
		BuildUI(Canvas);
	}
	return Super::RebuildWidget();
}

AStairGameMode* UStairWidgetBase::GetStairGameMode() const
{
	return Cast<AStairGameMode>(UGameplayStatics::GetGameMode(this));
}

AStairCharacter* UStairWidgetBase::GetStairPlayer() const
{
	return Cast<AStairCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}

UStairConfig* UStairWidgetBase::GetConfig() const
{
	if (AStairGameMode* GM = GetStairGameMode())
	{
		if (UStairConfig* C = GM->GetConfig())
		{
			return C;
		}
	}
	if (UStairGameInstance* GI = Cast<UStairGameInstance>(GetGameInstance()))
	{
		return GI->Config;
	}
	return nullptr;
}

UTextBlock* UStairWidgetBase::MakeText(const FString& Name, const FString& Text,
	int32 Size, const FLinearColor& Color, ETextJustify::Type Justify)
{
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), FName(*Name));
	T->SetText(FText::FromString(Text));

	FSlateFontInfo Font = T->GetFont();
	Font.Size = Size;
	if (UIFont)
	{
		Font.FontObject = UIFont;
	}
	Font.OutlineSettings.OutlineSize = 2;
	Font.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.9f);
	T->SetFont(Font);

	T->SetColorAndOpacity(FSlateColor(Color));
	T->SetJustification(Justify);
	return T;
}

UButton* UStairWidgetBase::MakeButton(const FString& Name, const FString& Label, int32 Size)
{
	UButton* B = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(), FName(*Name));

	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), FName(*(Name + TEXT("_Label"))));
	T->SetText(FText::FromString(Label));

	FSlateFontInfo Font = T->GetFont();
	Font.Size = Size;
	if (UIFont)
	{
		Font.FontObject = UIFont;
	}
	T->SetFont(Font);
	T->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.06f, 1.f)));
	T->SetJustification(ETextJustify::Center);

	B->AddChild(T);
	return B;
}

UBorder* UStairWidgetBase::MakeBox(const FString& Name, const FLinearColor& Color)
{
	UBorder* B = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), FName(*Name));
	B->SetBrushColor(Color);
	return B;
}

void UStairWidgetBase::Place(UCanvasPanel* Canvas, UWidget* W,
	const FVector2D& Anchor, const FVector2D& Alignment,
	const FVector2D& Position, const FVector2D& Size)
{
	UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(W);
	PanelSlot->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
	PanelSlot->SetAlignment(Alignment);
	PanelSlot->SetPosition(Position);
	PanelSlot->SetSize(Size);
	PanelSlot->SetAutoSize(false);
}

// =====================================================================
// タイトル
// =====================================================================

FString UStairWidgetBase::GetHowToText()
{
	return FString(
		TEXT("SPACE      ジャンプする\n")
		TEXT("A / D        押しっぱなしで斜め左・斜め右へ跳ぶ\n")
		TEXT("               どちらも押さなければ正面\n\n")
		TEXT("画面左のゲージが下から上へ昇る。\n")
		TEXT("上端の少し下にある緑の帯を通る瞬間に SPACE を押すと\n")
		TEXT("PERFECT になり、2段のぼれて足場も自動で作られる。\n")
		TEXT("黄色の帯なら GREAT で1段。外すと MISS でその場ジャンプ。\n\n")
		TEXT("赤いマスから跳ぶと一気に5段のぼれる。\n\n")
		TEXT("同じ足場で3回 MISS すると足場が崩れて終わり。\n")
		TEXT("段が無いところへ跳んで落ちても終わり。\n")
		TEXT("曲が終わるまで、どこまで高くのぼれるかを競う。"));
}

void UStairWidgetBase::BuildIris(UCanvasPanel* Canvas)
{
	// ★全画面に1枚だけ幕を置き、マテリアルで円をくり抜く。
	//   矩形を敷き詰める方式だと縁がドットに見えてしまう。
	IrisImage = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(), TEXT("IrisImage"));

	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/Stair/UI/M_Iris.M_Iris")))
	{
		IrisMat = UMaterialInstanceDynamic::Create(Base, this);
		if (IrisMat)
		{
			IrisImage->SetBrushFromMaterial(IrisMat);
		}
	}
	else
	{
		// マテリアルが無い場合は黒一色。開閉はしないが破綻はしない
		IrisImage->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.f));
	}

	Place(Canvas, IrisImage, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D::ZeroVector, FVector2D(8000.f, 8000.f));

	IrisImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UStairWidgetBase::SetIris(float Closed)
{
	if (!IrisMat)
	{
		return;
	}

	Closed = FMath::Clamp(Closed, 0.f, 1.f);

	// 半径 0 で真っ黒、1.1 で四隅まで開ききる
	IrisMat->SetScalarParameterValue(TEXT("Radius"), (1.f - Closed) * 1.12f);

	// 画面が横長でも真円になるよう、縦横比を渡す
	float Aspect = 1.7778f;
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D Size;
		GEngine->GameViewport->GetViewportSize(Size);
		if (Size.Y > 1.f)
		{
			Aspect = Size.X / Size.Y;
		}
	}
	IrisMat->SetScalarParameterValue(TEXT("Aspect"), Aspect);

	if (IrisImage)
	{
		// 開ききったら描画を止める
		IrisImage->SetVisibility(Closed <= 0.001f
			? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UStairWidgetBase::CloseIrisThen(TFunction<void()> Action)
{
	IrisAction = Action;
	IrisDir = 1;      // 閉じにいく
}

void UStairWidgetBase::TickIris(float DeltaSeconds)
{
	if (IrisDir == 0 || !IrisMat)
	{
		return;
	}

	const UStairConfig* C = GetConfig();
	const float Sec = C ? FMath::Max(0.05f, C->IrisSeconds) : 0.2f;
	const float Step = DeltaSeconds / Sec;

	if (IrisDir > 0)
	{
		// 閉じる
		IrisT = FMath::Min(1.f, IrisT + Step);
		SetIris(IrisT);

		if (IrisT >= 1.f)
		{
			IrisDir = 0;
			if (IrisAction)
			{
				// 暗転しきってから遷移する。切り替わりの瞬間を隠す
				TFunction<void()> A = IrisAction;
				IrisAction = nullptr;
				A();
			}
		}
	}
	else
	{
		// 開く
		IrisT = FMath::Max(0.f, IrisT - Step);
		SetIris(IrisT);
		if (IrisT <= 0.f)
		{
			IrisDir = 0;
		}
	}
}

void UStairWidgetBase::PlayButtonSound()
{
	if (const UStairConfig* C = GetConfig())
	{
		if (C->ButtonSound)
		{
			UGameplayStatics::PlaySound2D(this, C->ButtonSound, C->SystemSoundVolume);
		}
	}
}

void UStairTitleWidget::BuildUI(UCanvasPanel* Canvas)
{
	// ---- ロゴ ----
	// ★ネイティブクラスのCDOに入れた値はプロセスをまたいで残らないため、
	//   未設定なら直接ロードする。これをしないと白い四角になる。
	LogoImage = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(), TEXT("LogoImage"));

	const FVector2D LogoSize(1360.f, 635.f); // 元画像 1928x900 の比率

	// ★虹を流すマテリアルを優先して使う。無ければ静止画にする
	UObject* Resource = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/Stair/UI/M_LogoRainbow.M_LogoRainbow"));
	if (!Resource)
	{
		Resource = LogoTexture
			? ToRawPtr(LogoTexture)
			: LoadObject<UTexture2D>(nullptr, TEXT("/Game/Stair/UI/T_Logo.T_Logo"));
	}

	if (Resource)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Resource);
		Brush.ImageSize = LogoSize;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.TintColor = FSlateColor(FLinearColor::White);
		LogoImage->SetBrush(Brush);
	}
	else
	{
		// 何も無いときは白い四角を出さない
		LogoImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.f));
	}

	Place(Canvas, LogoImage, FVector2D(0.5f, 0.f), FVector2D(0.5f, 0.f),
		FVector2D(0.f, 30.f), LogoSize);
	MenuParts.Add(LogoImage);

	// ---- メニュー：プレイ と チュートリアル の2ボタン ----
	PlayButton = MakeButton(TEXT("PlayButton"), TEXT("プレイ"), 44);
	Place(Canvas, PlayButton, FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f),
		FVector2D(-190.f, -190.f), FVector2D(340.f, 118.f));
	MenuParts.Add(PlayButton);

	TutorialButton = MakeButton(TEXT("TutorialButton"), TEXT("遊び方"), 40);
	Place(Canvas, TutorialButton, FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f),
		FVector2D(190.f, -190.f), FVector2D(340.f, 118.f));
	MenuParts.Add(TutorialButton);

	// ---- 素材元の表記（常時表示）----
	// ★画面下端にぴったり付け、横幅は全体を覆う
	UBorder* CreditShade = MakeBox(TEXT("CreditShade"),
		FLinearColor(0.f, 0.f, 0.02f, 0.62f));
	Place(Canvas, CreditShade, FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f),
		FVector2D(0.f, 0.f), FVector2D(6000.f, 112.f));

	// ★「素材元」と一覧が詰まって見えたので行間を広げた
	UTextBlock* CreditHead = MakeText(TEXT("CreditHead"), TEXT("素材元"), 20,
		FLinearColor(0.60f, 0.65f, 0.73f, 1.f));
	Place(Canvas, CreditHead, FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f),
		FVector2D(0.f, -80.f), FVector2D(900.f, 28.f));

	// ★BGMは自作合成に差し替えたので魔王魂は外した。
	//   残っているのは効果音の出どころのみ。
	UTextBlock* Credits = MakeText(TEXT("Credits"),
		TEXT("OtoLogic　／　効果音ラボ　／　ニコニコモンズ"), 23,
		FLinearColor(0.80f, 0.84f, 0.90f, 1.f));
	Place(Canvas, Credits, FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f),
		FVector2D(0.f, -32.f), FVector2D(1400.f, 42.f));

	// ---- 遊び方パネル（文字）----
	UBorder* TutDim = MakeBox(TEXT("TutorialDim"), FLinearColor(0.f, 0.f, 0.02f, 0.86f));
	Place(Canvas, TutDim, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D::ZeroVector, FVector2D(6000.f, 4000.f));
	TutorialParts.Add(TutDim);

	UTextBlock* TutHead = MakeText(TEXT("HowToHead"), TEXT("あそびかた"), 50);
	Place(Canvas, TutHead, FVector2D(0.5f, 0.f), FVector2D(0.5f, 0.f),
		FVector2D(0.f, 60.f), FVector2D(900.f, 76.f));
	TutorialParts.Add(TutHead);

	UTextBlock* TutBody = MakeText(TEXT("HowToBody"), GetHowToText(), 27,
		FLinearColor(0.95f, 0.96f, 1.f, 1.f), ETextJustify::Left);
	Place(Canvas, TutBody, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D(0.f, -10.f), FVector2D(1180.f, 640.f));
	TutorialParts.Add(TutBody);

	// ---- 曲選択パネル ----
	// ★タイトルを暗くして重ねるだけ。背景の階段はそのまま流れ続ける
	UBorder* SongDim = MakeBox(TEXT("SongDim"), FLinearColor(0.f, 0.f, 0.02f, 0.72f));
	Place(Canvas, SongDim, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D::ZeroVector, FVector2D(6000.f, 4000.f));
	SongParts.Add(SongDim);

	UTextBlock* SongHead = MakeText(TEXT("SongHead"), TEXT("曲をえらぶ"), 50);
	Place(Canvas, SongHead, FVector2D(0.5f, 0.f), FVector2D(0.5f, 0.f),
		FVector2D(0.f, 70.f), FVector2D(900.f, 76.f));
	SongParts.Add(SongHead);

	{
		const UStairConfig* C = GetConfig();

		// 実際に登録されている曲だけ並べる
		const int32 Count = C ? FMath::Min(C->Songs.Num(), MaxSongs) : 0;
		const float Pitch = 88.f;
		const float Top = -((Count - 1) * Pitch) * 0.5f - 10.f;

		SongButtons.Reset();

		for (int32 i = 0; i < Count; ++i)
		{
			const FStairSong& S = C->Songs[i];

			FString Label = S.Title.IsEmpty()
				? FString::Printf(TEXT("曲 %d"), i + 1) : S.Title;

			const int32 Sec = FMath::RoundToInt(S.Duration);
			FString Sub = FString::Printf(TEXT("BPM %.0f    %d:%02d"),
				S.BPM, Sec / 60, Sec % 60);
			if (!S.Sound)
			{
				Sub += TEXT("    [音源なし]");
			}

			const float Y = Top + i * Pitch;

			UButton* B = MakeButton(FString::Printf(TEXT("Song%d"), i), Label, 28);
			Place(Canvas, B, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
				FVector2D(-130.f, Y), FVector2D(540.f, 74.f));
			SongButtons.Add(B);
			SongParts.Add(B);

			UTextBlock* Info = MakeText(FString::Printf(TEXT("SongInfo%d"), i),
				Sub, 21, FLinearColor(0.75f, 0.80f, 0.88f, 1.f), ETextJustify::Left);
			Place(Canvas, Info, FVector2D(0.5f, 0.5f), FVector2D(0.f, 0.5f),
				FVector2D(170.f, Y), FVector2D(440.f, 38.f));
			SongParts.Add(Info);
		}
	}

	// ---- 「もどる」は 遊び方 と 曲選択 で共用 ----
	BackButton = MakeButton(TEXT("BackButton"), TEXT("もどる"), 30);
	Place(Canvas, BackButton, FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f),
		FVector2D(0.f, -130.f), FVector2D(300.f, 86.f));
	TutorialParts.Add(BackButton);
	SongParts.Add(BackButton);

	// ★開閉の幕は最後に作る。他のすべてより手前に来るようにするため
	BuildIris(Canvas);
}

void UStairTitleWidget::NativeTick(const FGeometry& Geo, float DeltaSeconds)
{
	Super::NativeTick(Geo, DeltaSeconds);
	TickIris(DeltaSeconds);
}

void UStairTitleWidget::ShowPanel(EStairTitlePanel Panel)
{
	CurrentPanel = Panel;

	auto SetVis = [](const TArray<TObjectPtr<UWidget>>& Parts, bool bShow)
	{
		for (UWidget* W : Parts)
		{
			if (W)
			{
				W->SetVisibility(bShow ? ESlateVisibility::Visible
				                       : ESlateVisibility::Collapsed);
			}
		}
	};

	SetVis(MenuParts,     Panel == EStairTitlePanel::Menu);
	SetVis(TutorialParts, Panel == EStairTitlePanel::Tutorial);
	SetVis(SongParts,     Panel == EStairTitlePanel::SongSelect);

	// 「もどる」は両方の配列に入っているので、必要なら出し直す
	if (BackButton)
	{
		const bool bNeedBack = (Panel != EStairTitlePanel::Menu);
		BackButton->SetVisibility(bNeedBack ? ESlateVisibility::Visible
		                                    : ESlateVisibility::Collapsed);
	}
}

void UStairTitleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PlayButton)
	{
		PlayButton->OnClicked.AddUniqueDynamic(this, &UStairTitleWidget::OnPlayClicked);
	}
	if (TutorialButton)
	{
		TutorialButton->OnClicked.AddUniqueDynamic(this, &UStairTitleWidget::OnTutorialClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &UStairTitleWidget::OnBackClicked);
	}

	// 動的デリゲートは引数を取れないので、添字ごとに関数を用意して束ねる
	if (SongButtons.IsValidIndex(0) && SongButtons[0])
	{
		SongButtons[0]->OnClicked.AddUniqueDynamic(this, &UStairTitleWidget::OnSong0Clicked);
	}
	if (SongButtons.IsValidIndex(1) && SongButtons[1])
	{
		SongButtons[1]->OnClicked.AddUniqueDynamic(this, &UStairTitleWidget::OnSong1Clicked);
	}
	if (SongButtons.IsValidIndex(2) && SongButtons[2])
	{
		SongButtons[2]->OnClicked.AddUniqueDynamic(this, &UStairTitleWidget::OnSong2Clicked);
	}
	if (SongButtons.IsValidIndex(3) && SongButtons[3])
	{
		SongButtons[3]->OnClicked.AddUniqueDynamic(this, &UStairTitleWidget::OnSong3Clicked);
	}
	if (SongButtons.IsValidIndex(4) && SongButtons[4])
	{
		SongButtons[4]->OnClicked.AddUniqueDynamic(this, &UStairTitleWidget::OnSong4Clicked);
	}
	if (SongButtons.IsValidIndex(5) && SongButtons[5])
	{
		SongButtons[5]->OnClicked.AddUniqueDynamic(this, &UStairTitleWidget::OnSong5Clicked);
	}

	// リザルトの「曲をえらぶ」から戻ってきたら、いきなり曲選択を出す
	bool bJumpToSongs = false;
	if (UStairGameInstance* GI = Cast<UStairGameInstance>(GetGameInstance()))
	{
		bJumpToSongs = GI->bOpenSongSelectOnTitle;
		GI->bOpenSongSelectOnTitle = false;   // 一度きり
	}

	ShowPanel(bJumpToSongs ? EStairTitlePanel::SongSelect
	                       : EStairTitlePanel::Menu);
}

void UStairTitleWidget::OnPlayClicked()
{
	PlayButtonSound();
	ShowPanel(EStairTitlePanel::SongSelect);
}

void UStairTitleWidget::OnTutorialClicked()
{
	PlayButtonSound();
	ShowPanel(EStairTitlePanel::Tutorial);
}

void UStairTitleWidget::OnBackClicked()
{
	PlayButtonSound();
	ShowPanel(EStairTitlePanel::Menu);
}

void UStairTitleWidget::SelectSong(int32 Index)
{
	PlayButtonSound();
	if (UStairGameInstance* GI = Cast<UStairGameInstance>(GetGameInstance()))
	{
		GI->SelectedSongIndex = Index;
	}

	// ★丸が閉じきってからレベルを切り替える。切り替わりの瞬間を隠す
	TWeakObjectPtr<UStairTitleWidget> Weak(this);
	CloseIrisThen([Weak]()
	{
		if (Weak.IsValid())
		{
			UGameplayStatics::OpenLevel(Weak.Get(), TEXT("L_Game"));
		}
	});
}

void UStairTitleWidget::OnSong0Clicked() { SelectSong(0); }
void UStairTitleWidget::OnSong1Clicked() { SelectSong(1); }
void UStairTitleWidget::OnSong2Clicked() { SelectSong(2); }
void UStairTitleWidget::OnSong3Clicked() { SelectSong(3); }
void UStairTitleWidget::OnSong4Clicked() { SelectSong(4); }
void UStairTitleWidget::OnSong5Clicked() { SelectSong(5); }

// =====================================================================
// BGM選択
// =====================================================================

void UStairBGMSelectWidget::BuildUI(UCanvasPanel* Canvas)
{
	UBorder* BG = MakeBox(TEXT("BG"), FLinearColor(0.03f, 0.04f, 0.08f, 1.f));
	Place(Canvas, BG, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D::ZeroVector, FVector2D(4000.f, 3000.f));

	UTextBlock* Head = MakeText(TEXT("Head"), TEXT("曲をえらぶ"), 56);
	Place(Canvas, Head, FVector2D(0.5f, 0.f), FVector2D(0.5f, 0.f),
		FVector2D(0.f, 70.f), FVector2D(900.f, 90.f));

	UStairConfig* C = GetConfig();

	// 4曲ぶんのボタンを縦に並べる
	static const TCHAR* Fallback[4] = {
		TEXT("シャイニングスター"), TEXT("バーニングハート"),
		TEXT("ハルジオン"), TEXT("12345")
	};

	SongButtons.Reset();
	int32 MissingCount = 0;

	for (int32 i = 0; i < 4; ++i)
	{
		FString Label;
		bool bHasSound = false;

		if (C && C->Songs.IsValidIndex(i))
		{
			const FStairSong& S = C->Songs[i];
			Label = S.Title.IsEmpty() ? FString(Fallback[i]) : S.Title;
			bHasSound = (S.Sound != nullptr);
			Label += FString::Printf(TEXT("   BPM %.0f"), S.BPM);
		}
		else
		{
			Label = Fallback[i];
		}

		if (!bHasSound)
		{
			Label += TEXT("   [音源なし]");
			++MissingCount;
		}

		UButton* B = MakeButton(FString::Printf(TEXT("Song%d"), i), Label, 26);
		Place(Canvas, B, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
			FVector2D(0.f, -150.f + i * 96.f), FVector2D(760.f, 82.f));
		SongButtons.Add(B);
	}

	WarnText = MakeText(TEXT("WarnText"),
		(MissingCount > 0)
			? TEXT("※ [音源なし] の曲は無音で始まります。\n"
			       "  Content/Stair/Audio に取り込み、DA_StairConfig で割り当ててください。")
			: TEXT(""),
		20, FLinearColor(1.f, 0.75f, 0.35f, 1.f));
	Place(Canvas, WarnText, FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f),
		FVector2D(0.f, -150.f), FVector2D(1100.f, 80.f));

	UTextBlock* Credit = MakeText(TEXT("Credit"),
		TEXT("BGM: 魔王魂"), 20, FLinearColor(0.6f, 0.65f, 0.7f, 1.f));
	Place(Canvas, Credit, FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f),
		FVector2D(0.f, -60.f), FVector2D(600.f, 40.f));

	BackButton = MakeButton(TEXT("BackButton"), TEXT("もどる"), 24);
	Place(Canvas, BackButton, FVector2D(0.f, 1.f), FVector2D(0.f, 1.f),
		FVector2D(50.f, -50.f), FVector2D(200.f, 68.f));
}

void UStairBGMSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SongButtons.IsValidIndex(0) && SongButtons[0])
	{
		SongButtons[0]->OnClicked.AddUniqueDynamic(this, &UStairBGMSelectWidget::OnSong0Clicked);
	}
	if (SongButtons.IsValidIndex(1) && SongButtons[1])
	{
		SongButtons[1]->OnClicked.AddUniqueDynamic(this, &UStairBGMSelectWidget::OnSong1Clicked);
	}
	if (SongButtons.IsValidIndex(2) && SongButtons[2])
	{
		SongButtons[2]->OnClicked.AddUniqueDynamic(this, &UStairBGMSelectWidget::OnSong2Clicked);
	}
	if (SongButtons.IsValidIndex(3) && SongButtons[3])
	{
		SongButtons[3]->OnClicked.AddUniqueDynamic(this, &UStairBGMSelectWidget::OnSong3Clicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &UStairBGMSelectWidget::OnBackClicked);
	}
}

void UStairBGMSelectWidget::SelectSong(int32 Index)
{
	PlayButtonSound();
	if (UStairGameInstance* GI = Cast<UStairGameInstance>(GetGameInstance()))
	{
		GI->SelectedSongIndex = Index;
	}
	UGameplayStatics::OpenLevel(this, TEXT("L_Game"));
}

void UStairBGMSelectWidget::OnSong0Clicked() { SelectSong(0); }
void UStairBGMSelectWidget::OnSong1Clicked() { SelectSong(1); }
void UStairBGMSelectWidget::OnSong2Clicked() { SelectSong(2); }
void UStairBGMSelectWidget::OnSong3Clicked() { SelectSong(3); }

void UStairBGMSelectWidget::OnBackClicked()
{
	PlayButtonSound();
	UGameplayStatics::OpenLevel(this, TEXT("L_Title"));
}

// =====================================================================
// HUD
// =====================================================================

void UStairHUDWidget::BuildUI(UCanvasPanel* Canvas)
{
	RootCanvas = Canvas;

	// ---- 左上：緑のデジタル風タイマー ----
	UBorder* TimerBG = MakeBox(TEXT("TimerBG"), FLinearColor(0.f, 0.f, 0.f, 0.55f));
	Place(Canvas, TimerBG, FVector2D(0.f, 0.f), FVector2D(0.f, 0.f),
		FVector2D(40.f, 32.f), FVector2D(260.f, 78.f));

	TimerText = MakeText(TEXT("TimerText"), TEXT("0:00"), 52,
		FLinearColor(0.25f, 1.f, 0.45f, 1.f), ETextJustify::Center);
	Place(Canvas, TimerText, FVector2D(0.f, 0.f), FVector2D(0.f, 0.f),
		FVector2D(40.f, 38.f), FVector2D(260.f, 66.f));

	// ---- 右上：階段アイコン＋段数 ----
	UBorder* ScoreBG = MakeBox(TEXT("ScoreBG"), FLinearColor(0.f, 0.f, 0.f, 0.55f));
	Place(Canvas, ScoreBG, FVector2D(1.f, 0.f), FVector2D(1.f, 0.f),
		FVector2D(-40.f, 32.f), FVector2D(300.f, 78.f));

	// 階段アイコン（記号で代用）
	StairIcon = MakeText(TEXT("StairIcon"), TEXT("▛▘"), 40,
		FLinearColor(0.85f, 0.88f, 0.95f, 1.f), ETextJustify::Center);
	Place(Canvas, StairIcon, FVector2D(1.f, 0.f), FVector2D(1.f, 0.f),
		FVector2D(-230.f, 40.f), FVector2D(90.f, 62.f));

	ScoreText = MakeText(TEXT("ScoreText"), TEXT("0"), 50,
		FLinearColor::White, ETextJustify::Right);
	Place(Canvas, ScoreText, FVector2D(1.f, 0.f), FVector2D(1.f, 0.f),
		FVector2D(-56.f, 38.f), FVector2D(180.f, 66.f));

	// ---- 中央：カウントダウンと判定 ----
	CountdownText = MakeText(TEXT("CountdownText"), TEXT(""), 140);
	Place(Canvas, CountdownText, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D(0.f, -40.f), FVector2D(800.f, 200.f));

	JudgeText = MakeText(TEXT("JudgeText"), TEXT(""), 54);
	Place(Canvas, JudgeText, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D(0.f, 120.f), FVector2D(700.f, 80.f));

	// ---- 画面左中央：縦型の拍ゲージ ----
	// 下から上へカーソルが昇り、上端の少し下（緑）を通る瞬間が拍のジャスト。
	// 行き過ぎると最上部（赤）＝ MISS。
	const FVector2D Anchor(0.f, 0.5f);
	const FVector2D Align(0.5f, 0.5f);
	const float X = TrackLeftMargin;

	BeatTrack = MakeBox(TEXT("BeatTrack"), FLinearColor(0.f, 0.f, 0.f, 0.55f));
	Place(Canvas, BeatTrack, Anchor, Align,
		FVector2D(X, 0.f), FVector2D(TrackWidth, TrackHeight));

	GreatZone = MakeBox(TEXT("GreatZone"), FLinearColor(0.95f, 0.85f, 0.25f, 0.40f));
	Place(Canvas, GreatZone, Anchor, Align,
		FVector2D(X, FracToY(PerfectCenterFrac)), FVector2D(TrackWidth, 100.f));

	PerfectZone = MakeBox(TEXT("PerfectZone"), FLinearColor(0.25f, 1.f, 0.45f, 0.85f));
	Place(Canvas, PerfectZone, Anchor, Align,
		FVector2D(X, FracToY(PerfectCenterFrac)), FVector2D(TrackWidth, 40.f));

	// 最上部は MISS
	{
		const float Top = 1.f;
		const float Center = (MissStripFrac + Top) * 0.5f;
		const float H = TrackHeight * (Top - MissStripFrac);
		MissStrip = MakeBox(TEXT("MissStrip"), FLinearColor(0.9f, 0.18f, 0.20f, 0.65f));
		Place(Canvas, MissStrip, Anchor, Align,
			FVector2D(X, FracToY(Center)), FVector2D(TrackWidth, H));
	}

	// ★押した位置の残像。カーソルより細くして本体と区別する
	GhostMarks.Reset();
	for (int32 i = 0; i < 4; ++i)
	{
		UBorder* G = MakeBox(FString::Printf(TEXT("Ghost%d"), i),
			FLinearColor(1.f, 1.f, 1.f, 0.f));
		Place(Canvas, G, Anchor, Align,
			FVector2D(X, 0.f), FVector2D(TrackWidth + 26.f, 3.f));
		GhostMarks.Add(G);
	}

	BeatCursor = MakeBox(TEXT("BeatCursor"), FLinearColor(1.f, 1.f, 1.f, 0.97f));
	Place(Canvas, BeatCursor, Anchor, Align,
		FVector2D(X, 0.f), FVector2D(TrackWidth + 18.f, 6.f));

	// ---- 残りチャージ（撃てる回数）----
	ChargePips.Reset();
	for (int32 i = 0; i < 3; ++i)
	{
		UBorder* P = MakeBox(FString::Printf(TEXT("Pip%d"), i),
			FLinearColor(0.95f, 0.85f, 0.3f, 1.f));
		Place(Canvas, P, FVector2D(1.f, 1.f), FVector2D(1.f, 1.f),
			FVector2D(-40.f - i * 34.f, -40.f), FVector2D(24.f, 24.f));
		ChargePips.Add(P);
	}

	ReloadText = MakeText(TEXT("ReloadText"), TEXT(""), 22,
		FLinearColor(1.f, 0.7f, 0.3f, 1.f), ETextJustify::Right);
	Place(Canvas, ReloadText, FVector2D(1.f, 1.f), FVector2D(1.f, 1.f),
		FVector2D(-40.f, -74.f), FVector2D(320.f, 34.f));

	DirText = MakeText(TEXT("DirText"), TEXT("▲ 正面"), 40);
	Place(Canvas, DirText, FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f),
		FVector2D(0.f, -80.f), FVector2D(500.f, 60.f));

	// タイミング補正の表示（調整したときだけ出す）
	TunerText = MakeText(TEXT("TunerText"), TEXT(""), 24,
		FLinearColor(1.f, 0.95f, 0.5f, 1.f));
	Place(Canvas, TunerText, FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f),
		FVector2D(0.f, -26.f), FVector2D(1000.f, 40.f));

	// ---- コンボ ----
	// ★画面を縦に半分で割った、右側の長方形のど真ん中。
	//   アンカー(0.75, 0.5) が右半分の中心になる。
	ComboText = MakeText(TEXT("ComboText"), TEXT(""), 92,
		FLinearColor(1.f, 0.92f, 0.35f, 1.f));
	Place(Canvas, ComboText, FVector2D(0.75f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D(0.f, -20.f), FVector2D(520.f, 130.f));

	ComboLabel = MakeText(TEXT("ComboLabel"), TEXT(""), 34,
		FLinearColor(1.f, 0.98f, 0.75f, 1.f));
	Place(Canvas, ComboLabel, FVector2D(0.75f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D(0.f, 62.f), FVector2D(520.f, 50.f));

	// ★ゲーム開始時は閉じた状態から開く
	BuildIris(Canvas);
	SetIris(1.f);
}

void UStairHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// ★開始直後は閉じた状態なので、開いていく
	TickIris(InDeltaTime);

	AStairGameMode* GM = GetStairGameMode();
	AStairCharacter* P = GetStairPlayer();
	if (!GM)
	{
		return;
	}

	const EStairGameState S = GM->GetState();

	// ---- 左上タイマー ----
	if (TimerText)
	{
		const float R = GM->GetRemainingTime();
		const int32 M = FMath::FloorToInt(R / 60.f);
		const int32 Sec = FMath::FloorToInt(R) % 60;
		TimerText->SetText(FText::FromString(
			FString::Printf(TEXT("%d:%02d"), M, Sec)));
	}

	// ---- 右上段数 ----
	if (ScoreText)
	{
		ScoreText->SetText(FText::FromString(FString::FromInt(GM->GetScore())));
	}

	// ---- カウントダウン ----
	if (CountdownText)
	{
		// ★イントロ中は「テンポに合わせて押す」案内を同じ場所に出す
		const FString Intro = GM->GetIntroText();
		CountdownText->SetText(FText::FromString(
			Intro.IsEmpty() ? GM->GetCountdownText() : Intro));

		// 案内文は数字より小さくする
		FSlateFontInfo F = CountdownText->GetFont();
		F.Size = Intro.IsEmpty() ? 96 : 40;
		CountdownText->SetFont(F);
	}

	// ---- 判定表示 ----
	if (JudgeText && P)
	{
		if (P->GetTimeSinceJudge() < 0.45f && S == EStairGameState::Playing)
		{
			FString Txt;
			FLinearColor Col;
			switch (P->GetLastJudge())
			{
			case EStairJudge::Perfect:
				Txt = TEXT("PERFECT");
				Col = FLinearColor(0.25f, 1.f, 0.45f, 1.f);
				break;
			case EStairJudge::Great:
				Txt = TEXT("GREAT");
				Col = FLinearColor(0.95f, 0.85f, 0.25f, 1.f);
				break;
			default:
				Txt = TEXT("MISS");
				Col = FLinearColor(1.f, 0.35f, 0.35f, 1.f);
				break;
			}
			if (P->GetLastSteps() >= 5)
			{
				Txt += TEXT("  +5");
			}
			JudgeText->SetText(FText::FromString(Txt));
			JudgeText->SetColorAndOpacity(FSlateColor(Col));
		}
		else
		{
			JudgeText->SetText(FText::GetEmpty());
		}
	}

	// ---- 進行方向 ----
	if (DirText && P)
	{
		FString D;
		switch (P->GetDir())
		{
		case EStairDir::Left:  D = TEXT("◤  斜め左"); break;
		case EStairDir::Right: D = TEXT("斜め右  ◥"); break;
		default:               D = TEXT("▲ 正面");    break;
		}
		DirText->SetText(FText::FromString(
			(S == EStairGameState::Playing) ? D : FString()));
	}

	// ---- タイミング補正 と 譜面編集 の表示 ----
	if (TunerText)
	{
		// 譜面編集中はそちらを優先して出す
		const FString Chart = GM->GetChartText();
		TunerText->SetText(FText::FromString(
			Chart.IsEmpty() ? GM->GetTimingTunerText() : Chart));
	}

	// ---- コンボ ----
	if (ComboText && ComboLabel)
	{
		const UStairConfig* C = GetConfig();
		const int32 From = C ? C->ComboShowFrom : 3;
		const int32 Combo = GM->GetCombo();

		// 3コンボ未満は出さない
		const bool bShow = (Combo >= From) && (S == EStairGameState::Playing);

		ComboText->SetText(bShow
			? FText::FromString(FString::FromInt(Combo)) : FText::GetEmpty());
		ComboLabel->SetText(bShow
			? FText::FromString(TEXT("COMBO")) : FText::GetEmpty());

		if (bShow)
		{
			// 伸びるほど暖色から水色へ。10ごとに強く光らせる
			const float T = FMath::Clamp(Combo / 60.f, 0.f, 1.f);
			FLinearColor Col = FMath::Lerp(
				FLinearColor(1.f, 0.92f, 0.35f, 1.f),
				FLinearColor(0.55f, 0.95f, 1.f, 1.f), T);
			ComboText->SetColorAndOpacity(FSlateColor(Col));

			// 数字が変わった直後だけ少し大きくする
			const int32 Every = C ? FMath::Max(1, C->ComboSparkEvery) : 10;
			const float Scale = ((Combo % Every) == 0) ? 1.18f : 1.f;
			ComboText->SetRenderScale(FVector2D(Scale, Scale));
		}
	}

	// ---- 残りチャージ ----
	{
		const int32 Ch = GM->GetCharges();
		for (int32 i = 0; i < ChargePips.Num(); ++i)
		{
			if (!ChargePips[i]) { continue; }
			const bool bHave = (i < Ch);
			ChargePips[i]->SetBrushColor(bHave
				? FLinearColor(0.95f, 0.85f, 0.30f, 1.f)
				: FLinearColor(0.25f, 0.25f, 0.28f, 0.55f));
		}
		if (ReloadText)
		{
			ReloadText->SetText(GM->IsReloading()
				? FText::FromString(TEXT("リロード中"))
				: FText::GetEmpty());
		}
	}

	// ---- 縦型の拍ゲージ。判定と同じパラメータから帯の高さを作る ----
	if (PerfectZone && GreatZone && BeatCursor)
	{
		// ★50段のぼるとゲージは左へ去り、二度と戻らない。
		//   枠・帯・カーソル・残像を「まとめて」同じ X で動かす。
		//   一部だけ動かすと、枠だけが取り残されて残ってしまう。
		const float Exit = GM->GetGaugeExitAlpha();
		const float X = TrackLeftMargin - Exit * (TrackLeftMargin + TrackWidth + 120.f);

		// 枠
		if (UCanvasPanelSlot* TS = Cast<UCanvasPanelSlot>(BeatTrack->Slot))
		{
			TS->SetPosition(FVector2D(X, 0.f));
		}

		// 最上部の MISS 帯
		if (MissStrip)
		{
			if (UCanvasPanelSlot* MS = Cast<UCanvasPanelSlot>(MissStrip->Slot))
			{
				const float Center = (MissStripFrac + 1.f) * 0.5f;
				MS->SetPosition(FVector2D(X, FracToY(Center)));
			}
		}

		// 判定窓は「拍に対する割合」。ゲージ全体が1拍なのでそのまま高さになる
		const float PerfHalf = GM->GetPerfectZoneHalfWidth();
		const float GreatHalf = GM->GetGreatZoneHalfWidth();

		// 帯が最上部の MISS 帯に食い込まないよう上端で切る
		auto PlaceBand = [&](UBorder* Band, float Half)
		{
			const float Lo = PerfectCenterFrac - Half;
			const float Hi = FMath::Min(PerfectCenterFrac + Half, MissStripFrac);
			const float Center = (Lo + Hi) * 0.5f;
			const float H = FMath::Max(6.f, TrackHeight * (Hi - Lo));
			if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Band->Slot))
			{
				S->SetPosition(FVector2D(X, FracToY(Center)));
				S->SetSize(FVector2D(TrackWidth, H));
			}
		};

		PlaceBand(GreatZone, GreatHalf);
		PlaceBand(PerfectZone, PerfHalf);

		// ★テンポ合わせ中も動かす。
		//   ここで拍を掴んでもらうのが目的なので、
		//   メーターが止まっていては何に合わせるのか分からない。
		// カウントダウン中も押して拍を確かめられるので、メーターは動かす
		const EStairGameState St = GM->GetState();
		const bool bRunning = (St == EStairGameState::Playing
			|| St == EStairGameState::Intro
			|| St == EStairGameState::Countdown);

		const float Phase = bRunning ? GM->GetBeatPhase() : 0.f; // 0=ジャスト
		float H = PerfectCenterFrac + Phase;
		H -= FMath::FloorToFloat(H);                             // 0〜1に畳む

		if (UCanvasPanelSlot* CS = Cast<UCanvasPanelSlot>(BeatCursor->Slot))
		{
			CS->SetPosition(FVector2D(X, FracToY(H)));
		}

		// ジャストに近いほどカーソルを光らせる
		const bool bNearPerfect =
			bRunning && (FMath::Min(Phase, 1.f - Phase) <= PerfHalf);
		BeatCursor->SetBrushColor(bNearPerfect
			? FLinearColor(0.35f, 1.f, 0.55f, 1.f)
			: FLinearColor(1.f, 1.f, 1.f, 0.97f));

		// ---- 押した位置の残像 ----
		{
			const TArray<AStairGameMode::FStairGhost>& Gs = GM->GetGhosts();
			const UStairConfig* Cfg = GetConfig();
			const float Life = Cfg ? FMath::Max(0.1f, Cfg->GhostFadeSeconds) : 1.6f;

			for (int32 i = 0; i < GhostMarks.Num(); ++i)
			{
				UBorder* B = GhostMarks[i];
				if (!B) { continue; }

				if (!Gs.IsValidIndex(i) || !bRunning)
				{
					B->SetBrushColor(FLinearColor(1.f, 1.f, 1.f, 0.f));
					continue;
				}

				const AStairGameMode::FStairGhost& G = Gs[i];

				// 押したときの位相を、カーソルと同じ式で高さに直す
				float GH = PerfectCenterFrac + G.Phase;
				GH -= FMath::FloorToFloat(GH);

				if (UCanvasPanelSlot* GS = Cast<UCanvasPanelSlot>(B->Slot))
				{
					GS->SetPosition(FVector2D(X, FracToY(GH)));
				}

				// 判定ごとに色を分け、古いものほど薄くする
				FLinearColor Col;
				switch (G.Judge)
				{
				case EStairJudge::Perfect: Col = FLinearColor(0.35f, 1.f, 0.55f, 1.f); break;
				case EStairJudge::Great:   Col = FLinearColor(1.f, 0.88f, 0.30f, 1.f); break;
				default:                   Col = FLinearColor(1.f, 0.35f, 0.30f, 1.f); break;
				}
				Col.A = FMath::Clamp(1.f - (G.Age / Life), 0.f, 1.f) * 0.85f;
				B->SetBrushColor(Col);
			}
		}

		// カウントダウン中はゲージ全体を淡くして「まだ動かない」と分かるようにする。
		// 去るときは薄くせず、位置だけで画面外へ送る。
		const float Fade = bRunning ? 1.f : 0.35f;
		BeatTrack->SetRenderOpacity(Fade);
		GreatZone->SetRenderOpacity(Fade);
		PerfectZone->SetRenderOpacity(Fade);
		BeatCursor->SetRenderOpacity(Fade);
		if (MissStrip)
		{
			MissStrip->SetRenderOpacity(Fade);
		}
	}
}

// =====================================================================
// リザルト
// =====================================================================

void UStairResultWidget::BuildUI(UCanvasPanel* Canvas)
{
	UBorder* BG = MakeBox(TEXT("BG"), FLinearColor(0.f, 0.f, 0.f, 0.82f));
	Place(Canvas, BG, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D::ZeroVector, FVector2D(4000.f, 3000.f));

	ReasonText = MakeText(TEXT("ReasonText"), TEXT(""), 42,
		FLinearColor(1.f, 0.75f, 0.35f, 1.f));
	Place(Canvas, ReasonText, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D(0.f, -260.f), FVector2D(900.f, 70.f));

	UTextBlock* Label = MakeText(TEXT("Label"), TEXT("のぼった段数"), 30,
		FLinearColor(0.8f, 0.85f, 0.9f, 1.f));
	Place(Canvas, Label, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D(0.f, -170.f), FVector2D(800.f, 50.f));

	ScoreText = MakeText(TEXT("ScoreText"), TEXT("0"), 120,
		FLinearColor(0.35f, 1.f, 0.55f, 1.f));
	Place(Canvas, ScoreText, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D(0.f, -70.f), FVector2D(800.f, 170.f));

	DetailText = MakeText(TEXT("DetailText"), TEXT(""), 28);
	Place(Canvas, DetailText, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D(0.f, 50.f), FVector2D(800.f, 100.f));

	RetryButton = MakeButton(TEXT("RetryButton"), TEXT("もう一度"), 28);
	Place(Canvas, RetryButton, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D(-310.f, 180.f), FVector2D(280.f, 84.f));

	// 開閉の幕。他より手前に来るよう、ボタンより先に宣言しておく
	SelectButton = MakeButton(TEXT("SelectButton"), TEXT("曲をえらぶ"), 28);
	Place(Canvas, SelectButton, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D(0.f, 180.f), FVector2D(280.f, 84.f));

	TitleButton = MakeButton(TEXT("TitleButton"), TEXT("タイトルへ"), 28);
	Place(Canvas, TitleButton, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D(310.f, 180.f), FVector2D(280.f, 84.f));

	// ★開閉の幕は最後に作る。他のすべてより手前に来るようにするため
	BuildIris(Canvas);
	SetIris(1.f);   // 閉じた状態から開く
}

void UStairResultWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RetryButton)
	{
		RetryButton->OnClicked.AddUniqueDynamic(this, &UStairResultWidget::OnRetryClicked);
	}
	if (SelectButton)
	{
		SelectButton->OnClicked.AddUniqueDynamic(this, &UStairResultWidget::OnSelectClicked);
	}
	if (TitleButton)
	{
		TitleButton->OnClicked.AddUniqueDynamic(this, &UStairResultWidget::OnTitleClicked);
	}

	AStairGameMode* GM = GetStairGameMode();
	if (!GM)
	{
		return;
	}

	if (ScoreText)
	{
		ScoreText->SetText(FText::FromString(FString::FromInt(GM->GetScore())));
	}

	if (ReasonText)
	{
		FString R;
		switch (GM->GetEndReason())
		{
		case EStairEndReason::Fell:      R = TEXT("落下"); break;
		case EStairEndReason::Collapsed: R = TEXT("足場が崩れた"); break;
		case EStairEndReason::SongEnd:   R = TEXT("完走！"); break;
		default:                         R = TEXT(""); break;
		}
		ReasonText->SetText(FText::FromString(R));
	}

	if (DetailText)
	{
		// ★MISS は1つの数にまとめ、右に内訳を出す
		DetailText->SetText(FText::FromString(FString::Printf(
			TEXT("PERFECT %d    GREAT %d    MISS %d")
			TEXT("（ジャンプミス %d　ショットミス %d）\n最大コンボ  %d"),
			GM->GetPerfectCount(), GM->GetGreatCount(), GM->GetMissCount(),
			GM->GetJumpMissCount(), GM->GetShotMissCount(),
			GM->GetMaxCombo())));
	}

	if (UStairGameInstance* GI = Cast<UStairGameInstance>(GetGameInstance()))
	{
		GI->LastScore = GM->GetScore();
	}
}

void UStairResultWidget::NativeTick(const FGeometry& Geo, float DeltaSeconds)
{
	Super::NativeTick(Geo, DeltaSeconds);
	TickIris(DeltaSeconds);
}

// ★どのボタンも、丸が閉じきってから遷移させる。
//   切り替わりの瞬間を隠すことで、画面が飛んだ感じを無くす。
void UStairResultWidget::OnRetryClicked()
{
	PlayButtonSound();
	TWeakObjectPtr<UStairResultWidget> Weak(this);
	CloseIrisThen([Weak]()
	{
		if (Weak.IsValid())
		{
			if (AStairGameMode* GM = Weak->GetStairGameMode()) { GM->RetryGame(); }
		}
	});
}

void UStairResultWidget::OnSelectClicked()
{
	PlayButtonSound();
	TWeakObjectPtr<UStairResultWidget> Weak(this);
	CloseIrisThen([Weak]()
	{
		if (Weak.IsValid())
		{
			if (AStairGameMode* GM = Weak->GetStairGameMode()) { GM->GoToSongSelect(); }
		}
	});
}

void UStairResultWidget::OnTitleClicked()
{
	PlayButtonSound();
	TWeakObjectPtr<UStairResultWidget> Weak(this);
	CloseIrisThen([Weak]()
	{
		if (Weak.IsValid())
		{
			if (AStairGameMode* GM = Weak->GetStairGameMode()) { GM->GoToTitle(); }
		}
	});
}
