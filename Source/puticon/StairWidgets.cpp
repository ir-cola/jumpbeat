#include "StairWidgets.h"
#include "StairGameMode.h"
#include "StairCharacter.h"
#include "StairConfig.h"
#include "StairMusicClock.h"
#include "StairGameInstance.h"
#include "StairChartEditor.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
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

void UStairWidgetBase::PlaceFullScreen(UCanvasPanel* Canvas, UWidget* W)
{
	UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(W);
	PanelSlot->SetAutoSize(false);

	// 四隅にアンカーを張って、余白ゼロで画面と同じ形にする
	PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	PanelSlot->SetAlignment(FVector2D(0.f, 0.f));
	PanelSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));
}

// =====================================================================
// タイトル
// =====================================================================

FString UStairWidgetBase::GetHowToKeysText()
{
	return FString(
		TEXT("SPACE      正面へ跳ぶ\n")
		TEXT("A              左へ跳ぶ\n")
		TEXT("D              右へ跳ぶ\n")
		TEXT("Esc            ポーズ\n\n")
		TEXT("押した瞬間に、\n")
		TEXT("そのキーの方向へ跳びます。"));
}

FString UStairWidgetBase::GetHowToModesText()
{
	// ★1行は14文字くらいまで。左の列は幅が狭いので、
	//   長い行は自動で折り返されて読みにくくなる。
	return FString(
		TEXT("プレイ\n")
		TEXT("　選んだ曲を最後まで登ります。\n\n")
		TEXT("エンドレス\n")
		TEXT("　3曲が順番に流れ続けます。\n")
		TEXT("　落ちるまで終わりません。\n")
		TEXT("　地形は完全ランダムです。\n")
		TEXT("　向きは自由。タイミングだけ\n")
		TEXT("　合わせます。\n")
		TEXT("　記録はのぼった段数です。"));
}

FString UStairWidgetBase::GetHowToRulesText()
{
	return FString(
		TEXT("画面左のゲージを、音符が下から昇ってきます。\n")
		TEXT("緑の帯に重なった瞬間に押すと PERFECT。\n")
		TEXT("黄色の帯なら GREAT。外すと MISS です。\n\n")
		TEXT("音符の形が跳ぶ方向です。\n")
		TEXT("白いバー＝正面　　◀＝左　　▶＝右\n\n")
		TEXT("赤く染まった一列からは、5段先まで大ジャンプ。\n")
		TEXT("その先は谷になっているので、外すと落ちます。\n\n")
		TEXT("MISS するとその場で足踏みになります。\n")
		TEXT("同じ足場で3回 MISS すると、床が崩れて終わりです。\n")
		TEXT("穴に落ちても終わりです。\n\n")
		TEXT("曲が終わるまで、どこまで高く登れるかを競います。"));
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

	// ★画面と同じ形に広げる。
	//   ここを巨大な正方形にしていたため、マテリアルへ渡す UV が
	//   画面の縦横比と噛み合わず、丸が縦長の楕円になっていた。
	//   中心のごく狭い範囲が抜けたままになるのも同じ原因。
	PlaceFullScreen(Canvas, IrisImage);

	IrisImage->SetVisibility(ESlateVisibility::HitTestInvisible);

	// ★必ず真っ黒から始める。
	//   ここで初期値を入れておかないと、最初の1枚だけ
	//   マテリアル既定の半径で描かれて画面がちらつく。
	IrisT = 1.f;
	IrisDir = -1;
	IrisHold = -1.f;   // 最初の Tick で待ち時間を仕込む
	SetIris(1.f);
}

void UStairWidgetBase::SetIris(float Closed)
{
	if (!IrisMat)
	{
		return;
	}

	Closed = FMath::Clamp(Closed, 0.f, 1.f);

	// ★閉じきったら半径をマイナスまで送る。
	//   0 で止めるとマテリアルのぼかし幅ぶんだけ中心が抜けたままになり、
	//   画面の真ん中に小さな穴が残って完全な暗転にならない。
	IrisMat->SetScalarParameterValue(TEXT("Radius"),
		(1.f - Closed) * 1.15f - 0.03f);

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
	IrisHold = -1.f;
}

void UStairWidgetBase::TickIris(float DeltaSeconds)
{
	if (IrisDir == 0 || !IrisMat)
	{
		return;
	}

	const UStairConfig* C = GetConfig();
	const float Sec = C ? FMath::Max(0.05f, C->IrisSeconds) : 0.28f;
	const float Hold = C ? FMath::Max(0.f, C->IrisHoldSeconds) : 0.18f;

	// ★1フレームの進みに上限を置く。
	//   レベルの読み込みで詰まると DeltaSeconds が数百ミリ秒になり、
	//   開閉が1フレームで終わってワイプが見えなくなる。
	const float Dt = FMath::Min(DeltaSeconds, 0.033f);
	const float Step = Dt / Sec;

	// ---- 真っ黒のまま待っている最中 ----
	if (IrisHold >= 0.f)
	{
		SetIris(1.f);
		IrisHold -= Dt;
		if (IrisHold > 0.f)
		{
			return;
		}
		IrisHold = -1.f;

		if (IrisDir > 0)
		{
			// 閉じきって、待ちきった。ここで初めて切り替える
			IrisDir = 0;
			if (IrisAction)
			{
				TFunction<void()> A = IrisAction;
				IrisAction = nullptr;
				A();
			}
		}
		else
		{
			// ★開く側。1 のままだと次の Tick でまた待ちに入ってしまうので、
			//   ほんの少しだけ削って「待ちは済んだ」ことを表す。
			IrisT = FMath::Min(IrisT, 0.999f);
		}
		return;
	}

	if (IrisDir > 0)
	{
		// 閉じる
		IrisT = FMath::Min(1.f, IrisT + Step);
		SetIris(IrisT);

		if (IrisT >= 1.f)
		{
			// ★すぐには遷移しない。真っ黒の絵が確実に1枚出てからにする
			IrisHold = Hold;
		}
	}
	else
	{
		// ★開くときも、まず黒いまま少し待つ。
		//   切り替わった直後の重いフレームをここで吸収してから開く。
		if (IrisT >= 1.f && Hold > 0.f)
		{
			IrisHold = Hold;
			SetIris(1.f);
			return;
		}

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

	// ---- メニュー：プレイ / エンドレス / 遊び方 の3ボタン ----
	PlayButton = MakeButton(TEXT("PlayButton"), TEXT("プレイ"), 42);
	Place(Canvas, PlayButton, FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f),
		FVector2D(-350.f, -190.f), FVector2D(324.f, 118.f));
	MenuParts.Add(PlayButton);

	EndlessButton = MakeButton(TEXT("EndlessButton"), TEXT("エンドレス"), 38);
	Place(Canvas, EndlessButton, FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f),
		FVector2D(0.f, -190.f), FVector2D(324.f, 118.f));
	MenuParts.Add(EndlessButton);

	TutorialButton = MakeButton(TEXT("TutorialButton"), TEXT("遊び方"), 38);
	Place(Canvas, TutorialButton, FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f),
		FVector2D(350.f, -190.f), FVector2D(324.f, 118.f));
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

	// ★2列に分けて、上から下へ伸ばす。
	//   1列に全部並べると縦に長くなり、画面下の「もどる」に重なっていた。
	//   中央そろえではなく上そろえにして、行が増えても下へ食い込まないようにする。
	{
		const FVector2D Top(0.5f, 0.f);      // 画面上端が基準
		const FVector2D TopLeft(0.f, 0.f);   // 位置は左上を指す

		UTextBlock* KeysHead = MakeText(TEXT("HowToKeysHead"), TEXT("そうさ"), 26,
			FLinearColor(0.65f, 0.85f, 1.f, 1.f), ETextJustify::Left);
		Place(Canvas, KeysHead, Top, TopLeft, FVector2D(-560.f, 150.f),
			FVector2D(380.f, 36.f));
		TutorialParts.Add(KeysHead);

		UTextBlock* Keys = MakeText(TEXT("HowToKeys"), GetHowToKeysText(), 25,
			FLinearColor(0.95f, 0.96f, 1.f, 1.f), ETextJustify::Left);
		Place(Canvas, Keys, Top, TopLeft, FVector2D(-560.f, 196.f),
			FVector2D(400.f, 300.f));
		TutorialParts.Add(Keys);

		// ★左の列は「そうさ」が短いので、続けてモードの説明を置く
		UTextBlock* ModesHead = MakeText(TEXT("HowToModesHead"), TEXT("モード"), 26,
			FLinearColor(0.65f, 0.85f, 1.f, 1.f), ETextJustify::Left);
		Place(Canvas, ModesHead, Top, TopLeft, FVector2D(-560.f, 420.f),
			FVector2D(380.f, 36.f));
		TutorialParts.Add(ModesHead);

		UTextBlock* Modes = MakeText(TEXT("HowToModes"), GetHowToModesText(), 25,
			FLinearColor(0.95f, 0.96f, 1.f, 1.f), ETextJustify::Left);
		Place(Canvas, Modes, Top, TopLeft, FVector2D(-560.f, 466.f),
			FVector2D(400.f, 340.f));
		TutorialParts.Add(Modes);

		UTextBlock* RulesHead = MakeText(TEXT("HowToRulesHead"), TEXT("ルール"), 26,
			FLinearColor(0.65f, 0.85f, 1.f, 1.f), ETextJustify::Left);
		Place(Canvas, RulesHead, Top, TopLeft, FVector2D(-120.f, 150.f),
			FVector2D(700.f, 36.f));
		TutorialParts.Add(RulesHead);

		UTextBlock* Rules = MakeText(TEXT("HowToRules"), GetHowToRulesText(), 25,
			FLinearColor(0.95f, 0.96f, 1.f, 1.f), ETextJustify::Left);
		Place(Canvas, Rules, Top, TopLeft, FVector2D(-120.f, 196.f),
			FVector2D(700.f, 480.f));
		TutorialParts.Add(Rules);
	}

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

#if STAIR_SHOW_CHART_EDITOR
	// ---- 譜面づくりへの入口 ----
	// ★曲選択の右端に置く開発用のボタン。
	//   譜面は打ち終わって Saved/Charts に残っているので、
	//   完成版では出さない。作り直すときは
	//   StairWidgets.h の STAIR_SHOW_CHART_EDITOR を 1 に戻す。
	{
		ChartEditButton = MakeButton(TEXT("ChartEditButton"), TEXT("譜面をつくる"), 24);
		Place(Canvas, ChartEditButton, FVector2D(1.f, 0.5f), FVector2D(1.f, 0.5f),
			FVector2D(-60.f, -40.f), FVector2D(280.f, 72.f));
		SongParts.Add(ChartEditButton);

		UTextBlock* EditNote = MakeText(TEXT("ChartEditNote"),
			TEXT("開発用"), 17,
			FLinearColor(0.6f, 0.64f, 0.74f, 1.f));
		Place(Canvas, EditNote, FVector2D(1.f, 0.5f), FVector2D(1.f, 0.5f),
			FVector2D(-60.f, 16.f), FVector2D(280.f, 30.f));
		SongParts.Add(EditNote);
	}
#endif

	// ---- 「もどる」は 遊び方 と 曲選択 で共用 ----
	BackButton = MakeButton(TEXT("BackButton"), TEXT("もどる"), 30);
	Place(Canvas, BackButton, FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f),
		FVector2D(0.f, -130.f), FVector2D(300.f, 86.f));
	TutorialParts.Add(BackButton);
	SongParts.Add(BackButton);

	// ★開閉の幕は最後に作る。他のすべてより手前に来るようにするため
	BuildIris(Canvas);

	// ★起動直後の白い幕は、さらにその上。
	//   丸が開くより先に、まず白から明ける必要があるため。
	BootFade = MakeBox(TEXT("BootFade"), FLinearColor(1.f, 1.f, 1.f, 1.f));
	PlaceFullScreen(Canvas, BootFade);
	BootFade->SetVisibility(ESlateVisibility::Collapsed);
}

void UStairTitleWidget::NativeTick(const FGeometry& Geo, float DeltaSeconds)
{
	Super::NativeTick(Geo, DeltaSeconds);

	// ---- 起動直後の白い幕 ----
	if (BootFadeLeft > 0.f && BootFade)
	{
		const UStairConfig* C = GetConfig();
		const float Sec = C ? FMath::Max(0.1f, C->BootFadeSeconds) : 1.2f;

		// ★読み込みで詰まったフレームで一気に明けないよう、進みに上限を置く
		BootFadeLeft -= FMath::Min(DeltaSeconds, 0.033f);

		const float A = FMath::Clamp(BootFadeLeft / Sec, 0.f, 1.f);
		BootFade->SetBrushColor(FLinearColor(1.f, 1.f, 1.f, A));

		if (BootFadeLeft <= 0.f)
		{
			BootFade->SetVisibility(ESlateVisibility::Collapsed);
			BootFadeLeft = -1.f;
		}
		return;   // 白が明けきるまで丸は動かさない
	}

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
	if (EndlessButton)
	{
		EndlessButton->OnClicked.AddUniqueDynamic(this, &UStairTitleWidget::OnEndlessClicked);
	}
	if (TutorialButton)
	{
		TutorialButton->OnClicked.AddUniqueDynamic(this, &UStairTitleWidget::OnTutorialClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &UStairTitleWidget::OnBackClicked);
	}
#if STAIR_SHOW_CHART_EDITOR
	if (ChartEditButton)
	{
		ChartEditButton->OnClicked.AddUniqueDynamic(this, &UStairTitleWidget::OnChartEditClicked);
	}
#endif

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

		// ★起動して最初にタイトルを出すときだけ、白からのフェードイン。
		//   ゲームから戻ってきたときは、いつもどおりサークルワイプで開く。
		if (!GI->bBootDone)
		{
			GI->bBootDone = true;

			const UStairConfig* C = GetConfig();
			BootFadeLeft = C ? FMath::Max(0.1f, C->BootFadeSeconds) : 1.2f;

			if (BootFade)
			{
				BootFade->SetBrushColor(FLinearColor(1.f, 1.f, 1.f, 1.f));
				BootFade->SetVisibility(ESlateVisibility::HitTestInvisible);
			}

			// 丸は使わない。白が明けたらそのままタイトルが見えている状態にする
			IrisDir = 0;
			IrisT = 0.f;
			IrisHold = -1.f;
			SetIris(0.f);
		}
	}

	ShowPanel(bJumpToSongs ? EStairTitlePanel::SongSelect
	                       : EStairTitlePanel::Menu);
}

void UStairTitleWidget::OnPlayClicked()
{
	PlayButtonSound();
	ShowPanel(EStairTitlePanel::SongSelect);
}

void UStairTitleWidget::OnChartEditClicked()
{
	PlayButtonSound();

	APlayerController* PC = GetOwningPlayer();
	UStairChartEditWidget* W = CreateWidget<UStairChartEditWidget>(
		PC, UStairChartEditWidget::StaticClass());
	if (!W) { return; }

	// ★タイトルの上に重ねるだけ。レベルを切り替えないので、
	//   閉じればそのまま曲選択に戻れる。
	W->SetReturnWidget(this);
	W->AddToViewport(50);

	if (PC)
	{
		UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(PC, W, EMouseLockMode::DoNotLock);
	}
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

void UStairTitleWidget::OnEndlessClicked()
{
	PlayButtonSound();

	if (UStairGameInstance* GI = Cast<UStairGameInstance>(GetGameInstance()))
	{
		GI->bEndlessMode = true;
		GI->SelectedSongIndex = 0;
	}

	TWeakObjectPtr<UStairTitleWidget> Weak(this);
	CloseIrisThen([Weak]()
	{
		if (Weak.IsValid())
		{
			UGameplayStatics::OpenLevel(Weak.Get(), TEXT("L_Game"));
		}
	});
}

void UStairTitleWidget::SelectSong(int32 Index)
{
	PlayButtonSound();
	if (UStairGameInstance* GI = Cast<UStairGameInstance>(GetGameInstance()))
	{
		GI->SelectedSongIndex = Index;
		GI->bEndlessMode = false;   // 曲を選んだら通常プレイ
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

	// ★譜面の音符。下から昇ってきて判定帯に重なる。
	//   正面は白いバー、左右は矢印。バーと矢印を1組ずつ用意して切り替える。
	NoteMarks.Reset();
	NoteArrows.Reset();
	for (int32 i = 0; i < MaxNoteMarks; ++i)
	{
		UBorder* N = MakeBox(FString::Printf(TEXT("Note%d"), i),
			FLinearColor(1.f, 1.f, 1.f, 0.f));
		Place(Canvas, N, Anchor, Align,
			FVector2D(X, 0.f), FVector2D(TrackWidth + 22.f, 14.f));
		NoteMarks.Add(N);

		UTextBlock* A = MakeText(FString::Printf(TEXT("NoteArrow%d"), i),
			TEXT(""), 46, FLinearColor::White);
		Place(Canvas, A, Anchor, Align,
			FVector2D(X, 0.f), FVector2D(TrackWidth + 60.f, 54.f));
		NoteArrows.Add(A);
	}

	// ★次の曲を待っているあいだの案内（エンドレス）
	NextSongText = MakeText(TEXT("NextSongText"), TEXT(""), 40,
		FLinearColor(0.6f, 0.9f, 1.f, 1.f));
	Place(Canvas, NextSongText, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D(0.f, -40.f), FVector2D(1200.f, 80.f));

	// 譜面編集中であることを大きく出す
	ChartBanner = MakeText(TEXT("ChartBanner"), TEXT(""), 44,
		FLinearColor(1.f, 0.75f, 0.2f, 1.f));
	Place(Canvas, ChartBanner, FVector2D(0.5f, 0.f), FVector2D(0.5f, 0.f),
		FVector2D(0.f, 30.f), FVector2D(1000.f, 60.f));

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

	// ★リザルトへもサークルワイプで移る。
	//   リザルト側は閉じた状態から開くので、こちら側で先に閉じておかないと
	//   いきなり真っ黒になったように見えてしまう。
	//   Result に切り替わる瞬間に閉じ終わるよう、逆算して閉じ始める。
	if (S == EStairGameState::Finished && IrisDir <= 0 && IrisT <= 0.f)
	{
		const UStairConfig* Cfg = GetConfig();
		const float Need = (Cfg ? Cfg->IrisSeconds + Cfg->IrisHoldSeconds : 0.38f);

		if (GM->GetStateTime() >= GM->GetFinishedHold() - Need)
		{
			IrisDir = 1;
			IrisHold = -1.f;
		}
	}

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
		CountdownText->SetText(FText::FromString(GM->GetCountdownText()));
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
				// ★早かったのか遅かったのかを添える。
				//   どちらへ直せばよいのかが分からないと、GREAT のままになる。
				Txt = (GM->GetLastJudgeOffset() < 0.f)
					? TEXT("GREAT  FAST") : TEXT("GREAT  SLOW");
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
	{
		const FString Chart = GM->GetChartText();

		if (TunerText)
		{
			// 譜面編集中はそちらを優先して出す
			TunerText->SetText(FText::FromString(
				Chart.IsEmpty() ? GM->GetTimingTunerText() : Chart));
		}

		// ★編集中であることを画面上部に大きく出す。
		//   気づかずに遊び始めてしまうのを防ぐ
		if (ChartBanner)
		{
			ChartBanner->SetText(GM->IsCharting()
				? FText::FromString(TEXT("― 譜面作成中 ―"))
				: FText::GetEmpty());
		}
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

	// ---- 縦型のゲージ。判定と同じパラメータから帯の高さを作る ----
	if (PerfectZone && GreatZone)
	{
		// ★ゲージは最後まで出しっぱなし。譜面を読むゲームなので途中で消さない
		const float X = TrackLeftMargin;

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
			|| St == EStairGameState::Countdown);

		// ---- 譜面の音符を並べる ----
		// ★下から現れて判定帯へ昇っていく。太鼓の達人を90度倒した形。
		//   譜面が無い曲では、従来どおり往復するカーソルを使う。
		const bool bChart = GM->HasChart();
		{
			const float Look = FMath::Max(0.2f, NoteLookaheadSeconds);
			const int32 First = FMath::Max(0, GM->GetNextNoteIndex() - 1);

			for (int32 i = 0; i < NoteMarks.Num(); ++i)
			{
				UBorder* B = NoteMarks[i];
				if (!B) { continue; }

				UTextBlock* Arrow = NoteArrows.IsValidIndex(i) ? NoteArrows[i] : nullptr;

				auto HideNote = [&]()
				{
					B->SetBrushColor(FLinearColor(1.f, 1.f, 1.f, 0.f));
					if (Arrow) { Arrow->SetText(FText::GetEmpty()); }
				};

				const int32 Idx = First + i;
				if (!bChart || !bRunning || Idx >= GM->GetNoteCount())
				{
					HideNote();
					continue;
				}

				const float T = GM->GetNoteTimeFromNow(Idx);

				// 判定の瞬間に PerfectCenterFrac、Look 秒前に下端（0）
				const float NF = PerfectCenterFrac * (1.f - T / Look);
				if (NF < -0.05f || NF > 1.05f)
				{
					HideNote();
					continue;
				}

				const float NY = FracToY(NF);
				if (UCanvasPanelSlot* NS = Cast<UCanvasPanelSlot>(B->Slot))
				{
					NS->SetPosition(FVector2D(X, NY));
				}
				if (Arrow)
				{
					if (UCanvasPanelSlot* AS = Cast<UCanvasPanelSlot>(Arrow->Slot))
					{
						AS->SetPosition(FVector2D(X, NY));
					}
				}

				// 通り過ぎたものは薄くする
				const float Fade = (T < 0.f) ? 0.35f : 1.f;

				// ★正面と赤はバー、左右は矢印で見せる。
				//   ただしエンドレスは方向を問わないので、矢印は出さない。
				//   出すと「その向きに跳べ」と読めてしまい、
				//   ランダムな地形では穴に突っ込むことになる。
				const EStairNote Type = GM->IsEndless()
					? EStairNote::Forward : GM->GetNoteType(Idx);
				FString ArrowText;
				FLinearColor Col = FLinearColor(1.f, 1.f, 1.f, Fade);

				switch (Type)
				{
				case EStairNote::Left:
					ArrowText = TEXT("◀");
					Col.A = 0.f;                                  // バーは隠す
					break;
				case EStairNote::Right:
					ArrowText = TEXT("▶");
					Col.A = 0.f;
					break;
				default:
					// ★赤マスも白いバーのまま。押すキーは正面と同じなので、
					//   色を変えると「別の操作がいる」と読めてしまう。
					//   赤かどうかは足元の床を見れば分かる。
					Col = FLinearColor(1.f, 1.f, 1.f, Fade);      // 白いバー
					break;
				}

				B->SetBrushColor(Col);
				if (Arrow)
				{
					Arrow->SetText(FText::FromString(ArrowText));
					Arrow->SetColorAndOpacity(
						FSlateColor(FLinearColor(1.f, 1.f, 1.f, Fade)));
				}
			}
		}

		// ---- 押した位置の残像 ----
		{
			const TArray<AStairGameMode::FStairGhost>& Gs = GM->GetGhosts();
			const UStairConfig* Cfg = GetConfig();
			const float Life = Cfg ? FMath::Max(0.1f, Cfg->GhostFadeSeconds) : 1.6f;
			const float Look = FMath::Max(0.2f, NoteLookaheadSeconds);

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

				// ★押した瞬間に音符がどこにいたかを、音符と同じ式で置く。
				//   判定帯より下＝早すぎ、上＝遅すぎ、が一目で分かる。
				const float GH = FMath::Clamp(
					PerfectCenterFrac * (1.f + G.Offset / Look), -0.05f, 1.05f);

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

		// カウントダウン中はゲージ全体を淡くして「まだ動かない」と分かるようにする
		const float Fade = bRunning ? 1.f : 0.35f;
		BeatTrack->SetRenderOpacity(Fade);
		GreatZone->SetRenderOpacity(Fade);
		PerfectZone->SetRenderOpacity(Fade);
		if (MissStrip)
		{
			MissStrip->SetRenderOpacity(Fade);
		}
	}

	// ---- エンドレス：次の曲を待っているあいだの案内 ----
	if (NextSongText)
	{
		NextSongText->SetText(GM->IsBetweenSongs()
			? FText::FromString(FString::Printf(
				TEXT("♪ つぎの曲   %s"), *GM->GetSongTitle()))
			: FText::GetEmpty());
	}
}

// =====================================================================
// ポーズ
// =====================================================================

void UStairPauseWidget::BuildUI(UCanvasPanel* Canvas)
{
	UBorder* Dim = MakeBox(TEXT("PauseDim"), FLinearColor(0.f, 0.f, 0.02f, 0.78f));
	Place(Canvas, Dim, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D::ZeroVector, FVector2D(6000.f, 4000.f));

	UTextBlock* Head = MakeText(TEXT("PauseHead"), TEXT("ポーズ"), 64);
	Place(Canvas, Head, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D(0.f, -190.f), FVector2D(800.f, 90.f));

	ResumeButton = MakeButton(TEXT("ResumeButton"), TEXT("つづける"), 34);
	Place(Canvas, ResumeButton, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D(0.f, -50.f), FVector2D(420.f, 100.f));

	RestartButton = MakeButton(TEXT("RestartButton"), TEXT("やり直す"), 34);
	Place(Canvas, RestartButton, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D(0.f, 70.f), FVector2D(420.f, 100.f));

	TitleButton = MakeButton(TEXT("PauseTitleButton"), TEXT("タイトルへ"), 34);
	Place(Canvas, TitleButton, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D(0.f, 190.f), FVector2D(420.f, 100.f));

	UTextBlock* Hint = MakeText(TEXT("PauseHint"),
		TEXT("Esc でも戻れます"), 22, FLinearColor(0.7f, 0.75f, 0.85f, 1.f));
	Place(Canvas, Hint, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D(0.f, 280.f), FVector2D(600.f, 40.f));

	// ★ここからのレベル切り替えもサークルワイプにする。
	//   ポーズだけ幕を持っていなかったので、
	//   「やり直す」「タイトルへ」が一瞬で切り替わっていた。
	BuildIris(Canvas);

	// この画面は開いた状態で出す。閉じるのは遷移のときだけ
	IrisDir = 0;
	IrisT = 0.f;
	IrisHold = -1.f;
	SetIris(0.f);
}

void UStairPauseWidget::NativeTick(const FGeometry& Geo, float DeltaSeconds)
{
	Super::NativeTick(Geo, DeltaSeconds);

	// ★ゲームが止まっていても Slate は動くので、丸はここで進められる
	TickIris(DeltaSeconds);
}

void UStairPauseWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddUniqueDynamic(this, &UStairPauseWidget::OnResumeClicked);
	}
	if (RestartButton)
	{
		RestartButton->OnClicked.AddUniqueDynamic(this, &UStairPauseWidget::OnRestartClicked);
	}
	if (TitleButton)
	{
		TitleButton->OnClicked.AddUniqueDynamic(this, &UStairPauseWidget::OnTitleClicked);
	}
}

void UStairPauseWidget::OnResumeClicked()
{
	PlayButtonSound();
	if (AStairGameMode* GM = GetStairGameMode()) { GM->TogglePause(); }
}

void UStairPauseWidget::OnRestartClicked()
{
	PlayButtonSound();

	// ★丸が閉じきってから切り替える
	TWeakObjectPtr<UStairPauseWidget> Weak(this);
	CloseIrisThen([Weak]()
	{
		if (!Weak.IsValid()) { return; }
		if (AStairGameMode* GM = Weak->GetStairGameMode())
		{
			// ★止めたまま遷移すると次のレベルも止まったままになる
			GM->TogglePause();
			GM->RetryGame();
		}
	});
}

void UStairPauseWidget::OnTitleClicked()
{
	PlayButtonSound();

	TWeakObjectPtr<UStairPauseWidget> Weak(this);
	CloseIrisThen([Weak]()
	{
		if (!Weak.IsValid()) { return; }
		if (AStairGameMode* GM = Weak->GetStairGameMode())
		{
			GM->TogglePause();
			GM->GoToTitle();
		}
	});
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
		if (GM->IsEndless())
		{
			R = TEXT("エンドレス　") + R;
		}
		ReasonText->SetText(FText::FromString(R));
	}

	if (DetailText)
	{
		FString D = FString::Printf(
			TEXT("PERFECT %d    GREAT %d    MISS %d\n最大コンボ  %d"),
			GM->GetPerfectCount(), GM->GetGreatCount(), GM->GetMissCount(),
			GM->GetMaxCombo());

		// ★エンドレスは何曲ぶん持ちこたえたかが手応えになる
		if (GM->IsEndless())
		{
			D += FString::Printf(TEXT("\n流れた曲  %d 曲"), GM->GetSongsPlayed());
		}
		DetailText->SetText(FText::FromString(D));
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
