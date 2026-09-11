#include "StairChartEditor.h"
#include "StairConfig.h"
#include "StairChartFile.h"
#include "StairMenuGameMode.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Kismet/GameplayStatics.h"

// 列の並び。クリック位置から「どの向きに跳ぶか」へ換算するのに使う
static const EStairNote ColType[4] = {
	EStairNote::Left, EStairNote::Forward, EStairNote::Right, EStairNote::Red
};
static const TCHAR* ColName[4] = { TEXT("左 A"), TEXT("正面 SPACE"), TEXT("右 D"), TEXT("赤 W") };

void UStairChartEditWidget::BuildUI(UCanvasPanel* Canvas)
{
	UBorder* BG = MakeBox(TEXT("EditBG"), FLinearColor(0.04f, 0.05f, 0.08f, 1.f));
	Place(Canvas, BG, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f),
		FVector2D::ZeroVector, FVector2D(6000.f, 4000.f));

	TitleText = MakeText(TEXT("EditTitle"), TEXT("譜面をつくる"), 36,
		FLinearColor::White, ETextJustify::Left);
	Place(Canvas, TitleText, FVector2D(0.f, 0.f), FVector2D(0.f, 0.f),
		FVector2D(40.f, 34.f), FVector2D(360.f, 52.f));

	InfoText = MakeText(TEXT("EditInfo"), TEXT(""), 19,
		FLinearColor(0.75f, 0.8f, 0.9f, 1.f), ETextJustify::Left);
	Place(Canvas, InfoText, FVector2D(0.f, 0.f), FVector2D(0.f, 0.f),
		FVector2D(40.f, 100.f), FVector2D(360.f, 520.f));

	// ---- 列の見出し ----
	for (int32 c = 0; c < NumCols; ++c)
	{
		UTextBlock* H = MakeText(FString::Printf(TEXT("ColHead%d"), c),
			ColName[c], 21, FLinearColor(0.85f, 0.9f, 1.f, 1.f));
		Place(Canvas, H, FVector2D(0.f, 0.f), FVector2D(0.5f, 0.5f),
			FVector2D(GridLeft + CellW * (c + 0.5f), GridTop - 26.f),
			FVector2D(CellW, 34.f));
	}

	// ---- 格子。見えている範囲だけ用意する ----
	Cells.Reset();
	RowLabels.Reset();
	for (int32 r = 0; r < VisibleRows; ++r)
	{
		UTextBlock* L = MakeText(FString::Printf(TEXT("RowLabel%d"), r),
			TEXT(""), 17, FLinearColor(0.6f, 0.66f, 0.78f, 1.f), ETextJustify::Right);
		Place(Canvas, L, FVector2D(0.f, 0.f), FVector2D(1.f, 0.5f),
			FVector2D(GridLeft - 14.f, GridTop + CellH * (r + 0.5f)),
			FVector2D(150.f, 28.f));
		RowLabels.Add(L);

		for (int32 c = 0; c < NumCols; ++c)
		{
			UBorder* B = MakeBox(FString::Printf(TEXT("Cell%d_%d"), r, c),
				FLinearColor(0.1f, 0.12f, 0.17f, 1.f));
			Place(Canvas, B, FVector2D(0.f, 0.f), FVector2D(0.f, 0.f),
				FVector2D(GridLeft + CellW * c + 2.f, GridTop + CellH * r + 2.f),
				FVector2D(CellW - 4.f, CellH - 4.f));
			Cells.Add(B);
		}
	}

	// ---- 再生位置の線 ----
	PlayHead = MakeBox(TEXT("PlayHead"), FLinearColor(1.f, 0.9f, 0.3f, 0.95f));
	Place(Canvas, PlayHead, FVector2D(0.f, 0.f), FVector2D(0.f, 0.5f),
		FVector2D(GridLeft - 8.f, GridTop), FVector2D(CellW * NumCols + 16.f, 3.f));

	// ---- ボタン ----
	PlayButton = MakeButton(TEXT("EditPlay"), TEXT("再生 / 停止"), 23);
	Place(Canvas, PlayButton, FVector2D(0.f, 1.f), FVector2D(0.f, 1.f),
		FVector2D(40.f, -180.f), FVector2D(300.f, 66.f));

	SaveButton = MakeButton(TEXT("EditSave"), TEXT("書き出す"), 23);
	Place(Canvas, SaveButton, FVector2D(0.f, 1.f), FVector2D(0.f, 1.f),
		FVector2D(40.f, -106.f), FVector2D(300.f, 66.f));

	BackButton = MakeButton(TEXT("EditBack"), TEXT("もどる"), 23);
	Place(Canvas, BackButton, FVector2D(0.f, 1.f), FVector2D(0.f, 1.f),
		FVector2D(40.f, -32.f), FVector2D(300.f, 66.f));
}

void UStairChartEditWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PlayButton) { PlayButton->OnClicked.AddUniqueDynamic(this, &UStairChartEditWidget::OnPlayClicked); }
	if (SaveButton) { SaveButton->OnClicked.AddUniqueDynamic(this, &UStairChartEditWidget::OnSaveClicked); }
	if (BackButton) { BackButton->OnClicked.AddUniqueDynamic(this, &UStairChartEditWidget::OnBackClicked); }

	LoadChart();

	// ★タイトルのBGMを止める。譜面を聴きながら打つので混ざると使いものにならない
	if (AStairTitleGameMode* TGM = Cast<AStairTitleGameMode>(
		UGameplayStatics::GetGameMode(this)))
	{
		TGM->SetBGMPaused(true);
	}

	// ★キー操作（スクロールと再生）を受けるためにフォーカスを取る
	SetIsFocusable(true);
	SetKeyboardFocus();

	RefreshGrid();
}

void UStairChartEditWidget::NativeDestruct()
{
	// ★閉じ方に関わらず必ず残す。「もどる」を押し忘れても消えない
	SaveChart();

	if (Audio)
	{
		Audio->Stop();
		Audio->DestroyComponent();
		Audio = nullptr;
	}

	if (AStairTitleGameMode* TGM = Cast<AStairTitleGameMode>(
		UGameplayStatics::GetGameMode(this)))
	{
		TGM->SetBGMPaused(false);
	}

	Super::NativeDestruct();
}

void UStairChartEditWidget::LoadChart()
{
	// ★書き出したものがあればそちらが正。Config は製品版に焼いた分の控え
	if (StairChartFile::Load(SongIndex, Notes))
	{
		return;
	}

	Notes.Reset();
	if (const UStairConfig* C = GetConfig())
	{
		if (C->Songs.IsValidIndex(SongIndex))
		{
			Notes = C->Songs[SongIndex].Notes;
		}
	}
}

bool UStairChartEditWidget::SaveChart()
{
	const UStairConfig* C = GetConfig();
	if (!C || !C->Songs.IsValidIndex(SongIndex))
	{
		return false;
	}

	return StairChartFile::Save(SongIndex, Notes,
		C->Songs[SongIndex].Title, C->Songs[SongIndex].BPM, C->ChartSubdivision);
}

int32 UStairChartEditWidget::FindNote(int32 InSlot, EStairNote Type) const
{
	for (int32 i = 0; i < Notes.Num(); ++i)
	{
		if (Notes[i].Slot == InSlot && Notes[i].Type == Type)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void UStairChartEditWidget::ToggleNote(int32 InSlot, EStairNote Type)
{
	const int32 Found = FindNote(InSlot, Type);
	if (Found != INDEX_NONE)
	{
		Notes.RemoveAt(Found);
		return;
	}

	// ★同じ時刻に2つは置けない。跳ぶ向きは1回に1つしか選べないため、
	//   別の種類が既にあれば置き換える。
	for (int32 i = Notes.Num() - 1; i >= 0; --i)
	{
		if (Notes[i].Slot == InSlot)
		{
			Notes.RemoveAt(i);
		}
	}

	FStairChartNote N;
	N.Slot = InSlot;
	N.Type = Type;
	Notes.Add(N);
	Notes.Sort();
}

void UStairChartEditWidget::RefreshGrid()
{
	const UStairConfig* C = GetConfig();
	const int32 Sub = C ? FMath::Max(1, C->ChartSubdivision) : 4;

	for (int32 r = 0; r < VisibleRows; ++r)
	{
		const int32 SlotNo = TopSlot + r;

		if (RowLabels.IsValidIndex(r) && RowLabels[r])
		{
			// 拍の頭だけ番号を出す。間は点で示す
			RowLabels[r]->SetText(FText::FromString(
				(SlotNo % Sub == 0)
					? FString::Printf(TEXT("%d"), SlotNo / Sub)
					: TEXT("・")));
		}

		for (int32 c = 0; c < NumCols; ++c)
		{
			const int32 Idx = r * NumCols + c;
			if (!Cells.IsValidIndex(Idx) || !Cells[Idx]) { continue; }

			const bool bHas = FindNote(SlotNo, ColType[c]) != INDEX_NONE;

			FLinearColor Col;
			if (bHas)
			{
				switch (ColType[c])
				{
				case EStairNote::Left:  Col = FLinearColor(0.35f, 0.70f, 1.00f, 1.f); break;
				case EStairNote::Right: Col = FLinearColor(1.00f, 0.62f, 0.20f, 1.f); break;
				case EStairNote::Red:   Col = FLinearColor(1.00f, 0.20f, 0.22f, 1.f); break;
				default:                Col = FLinearColor(1.00f, 1.00f, 1.00f, 1.f); break;
				}
			}
			else
			{
				// 拍の頭は少し明るくして、リズムが読めるようにする
				Col = (SlotNo % Sub == 0)
					? FLinearColor(0.17f, 0.20f, 0.27f, 1.f)
					: FLinearColor(0.10f, 0.12f, 0.17f, 1.f);
			}
			Cells[Idx]->SetBrushColor(Col);
		}
	}

	if (InfoText)
	{
		FString SongTitle = TEXT("（曲がありません）");
		float BPM = 0.f;
		int32 SongCount = 0;
		if (C)
		{
			SongCount = C->Songs.Num();
			if (C->Songs.IsValidIndex(SongIndex))
			{
				SongTitle = C->Songs[SongIndex].Title;
				BPM = C->Songs[SongIndex].BPM;
			}
		}

		InfoText->SetText(FText::FromString(FString::Printf(
			TEXT("%s  (%d/%d)\n")
			TEXT("BPM %.0f    1拍を %d 分割\n")
			TEXT("音符 %d 個\n\n")
			TEXT("マスをクリック    置く／消す\n")
			TEXT("ホイール・↑↓     スクロール\n")
			TEXT("PageUp/PageDown   1画面ぶん\n")
			TEXT("SPACE             再生／停止\n")
			TEXT("←／→             曲を変える\n\n")
			TEXT("同じ行に置けるのは1つだけ。\n")
			TEXT("跳ぶ向きは1回に1つのため。\n\n")
			TEXT("書き出し先は Saved/Charts。\n")
			TEXT("Tools/import_chart.py で\n")
			TEXT("Config へ取り込む。"),
			*SongTitle, SongIndex + 1, FMath::Max(1, SongCount),
			BPM, Sub, Notes.Num())));
	}
}

FReply UStairChartEditWidget::NativeOnMouseButtonDown(
	const FGeometry& Geo, const FPointerEvent& Ev)
{
	// ★セルごとにボタンを置かず、クリック位置から行と列を割り出す。
	//   96個のボタンを並べるより軽く、行数を変えても壊れない。
	const FVector2D Local = Geo.AbsoluteToLocal(Ev.GetScreenSpacePosition());

	const float RX = Local.X - GridLeft;
	const float RY = Local.Y - GridTop;

	const int32 Col = (RX >= 0.f) ? FMath::FloorToInt(RX / CellW) : -1;
	const int32 Row = (RY >= 0.f) ? FMath::FloorToInt(RY / CellH) : -1;

	if (Col >= 0 && Col < NumCols && Row >= 0 && Row < VisibleRows)
	{
		ToggleNote(TopSlot + Row, ColType[Col]);
		RefreshGrid();
	}

	// クリックのたびにフォーカスを取り直す。キー操作を効かせ続けるため
	return FReply::Handled().SetUserFocus(TakeWidget(), EFocusCause::Mouse);
}

FReply UStairChartEditWidget::NativeOnMouseWheel(
	const FGeometry& Geo, const FPointerEvent& Ev)
{
	TopSlot = FMath::Max(0, TopSlot - FMath::RoundToInt(Ev.GetWheelDelta()) * 2);
	RefreshGrid();
	return FReply::Handled();
}

FReply UStairChartEditWidget::NativeOnKeyDown(
	const FGeometry& Geo, const FKeyEvent& Ev)
{
	const FKey K = Ev.GetKey();

	if (K == EKeys::Up)       { TopSlot = FMath::Max(0, TopSlot - 1); RefreshGrid(); return FReply::Handled(); }
	if (K == EKeys::Down)     { ++TopSlot; RefreshGrid(); return FReply::Handled(); }
	if (K == EKeys::PageUp)   { TopSlot = FMath::Max(0, TopSlot - VisibleRows); RefreshGrid(); return FReply::Handled(); }
	if (K == EKeys::PageDown) { TopSlot += VisibleRows; RefreshGrid(); return FReply::Handled(); }
	if (K == EKeys::Home)     { TopSlot = 0; RefreshGrid(); return FReply::Handled(); }
	if (K == EKeys::SpaceBar) { TogglePlay(); return FReply::Handled(); }
	if (K == EKeys::Left)     { SwitchSong(-1); return FReply::Handled(); }
	if (K == EKeys::Right)    { SwitchSong(+1); return FReply::Handled(); }

	return FReply::Unhandled();
}

void UStairChartEditWidget::SwitchSong(int32 Delta)
{
	const UStairConfig* C = GetConfig();
	if (!C || C->Songs.Num() <= 1) { return; }

	// ★移る前に必ず残す。書き出す前に消えると作り直しになる
	SaveChart();

	bPlaying = false;
	if (Audio)
	{
		// 曲が変われば音源も変わる。作り直させる
		Audio->Stop();
		Audio->DestroyComponent();
		Audio = nullptr;
	}

	SongIndex = (SongIndex + Delta + C->Songs.Num()) % C->Songs.Num();

	LoadChart();

	TopSlot = 0;
	PlayTime = 0.f;
	RefreshGrid();
}

void UStairChartEditWidget::TogglePlay()
{
	const UStairConfig* C = GetConfig();
	if (!C || !C->Songs.IsValidIndex(SongIndex) || !C->Songs[SongIndex].Sound)
	{
		return;
	}

	if (bPlaying)
	{
		bPlaying = false;
		if (Audio) { Audio->Stop(); }
		return;
	}

	// ★いま画面の上端に見えている位置から鳴らす。
	//   打ち込みたい場所だけを繰り返し聴けるようにするため。
	const int32 Sub = FMath::Max(1, C->ChartSubdivision);
	const float Beat = 60.f / FMath::Max(1.f, C->Songs[SongIndex].BPM) * C->BeatsPerJudge;
	PlayTime = (float(TopSlot) / Sub) * Beat + C->Songs[SongIndex].BeatOffset;

	if (!Audio)
	{
		Audio = UGameplayStatics::CreateSound2D(this, C->Songs[SongIndex].Sound,
			1.f, 1.f, 0.f, nullptr, false, false);
	}
	if (Audio)
	{
		Audio->Play(FMath::Max(0.f, PlayTime));
		bPlaying = true;
	}
}

void UStairChartEditWidget::NativeTick(const FGeometry& Geo, float DeltaSeconds)
{
	Super::NativeTick(Geo, DeltaSeconds);

	const UStairConfig* C = GetConfig();
	if (!C || !C->Songs.IsValidIndex(SongIndex) || !PlayHead)
	{
		return;
	}

	const int32 Sub = FMath::Max(1, C->ChartSubdivision);
	const float Beat = 60.f / FMath::Max(1.f, C->Songs[SongIndex].BPM) * C->BeatsPerJudge;
	const float Offset = C->Songs[SongIndex].BeatOffset;

	if (bPlaying)
	{
		PlayTime += DeltaSeconds;

		// 再生位置が下端に近づいたら、追いかけてスクロールする
		const float SlotAt = (PlayTime - Offset) / Beat * Sub;
		if (SlotAt > TopSlot + VisibleRows - 6)
		{
			TopSlot = FMath::Max(0, FMath::RoundToInt(SlotAt) - VisibleRows + 6);
			RefreshGrid();
		}
	}

	const float SlotNow = (PlayTime - Offset) / Beat * Sub;
	const float Y = GridTop + (SlotNow - TopSlot) * CellH;

	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(PlayHead->Slot))
	{
		S->SetPosition(FVector2D(GridLeft - 8.f, Y));
	}
	PlayHead->SetVisibility(
		(SlotNow >= TopSlot - 1.f && SlotNow <= TopSlot + VisibleRows)
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}

void UStairChartEditWidget::ExportChart()
{
	const UStairConfig* C = GetConfig();
	if (!C || !C->Songs.IsValidIndex(SongIndex))
	{
		return;
	}

	const bool bOK = SaveChart();
	const FString Path = StairChartFile::PathFor(SongIndex);

	UE_LOG(LogTemp, Warning, TEXT("[譜面] %s : %s （%d 個）"),
		bOK ? TEXT("書き出しました") : TEXT("書き出せませんでした"),
		*Path, Notes.Num());

	if (InfoText)
	{
		InfoText->SetText(FText::FromString(FString::Printf(
			TEXT("%s\n\n%s\n\n音符 %d 個\n\n何か操作すると元の表示に戻ります。"),
			bOK ? TEXT("書き出しました") : TEXT("書き出せませんでした"),
			*Path, Notes.Num())));
	}
}

void UStairChartEditWidget::OnPlayClicked()
{
	PlayButtonSound();
	TogglePlay();
	SetKeyboardFocus();
}

void UStairChartEditWidget::OnSaveClicked()
{
	PlayButtonSound();
	ExportChart();
	SetKeyboardFocus();
}

void UStairChartEditWidget::OnBackClicked()
{
	PlayButtonSound();

	bPlaying = false;
	if (Audio) { Audio->Stop(); }

	APlayerController* PC = GetOwningPlayer();
	RemoveFromParent();

	// 操作をタイトルへ返す
	if (PC)
	{
		UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(
			PC, ReturnWidget.Get(), EMouseLockMode::DoNotLock);
	}
}
