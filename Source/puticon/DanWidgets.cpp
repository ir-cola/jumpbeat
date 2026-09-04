#include "DanWidgets.h"
#include "DanGameMode.h"
#include "DanGameParams.h"
#include "DanSaveGame.h"
#include "ChargeMinigameBase.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Components/ProgressBar.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

// =====================================================================
// 共通基底
// =====================================================================

ADanGameMode* UDanWidgetBase::GetDanGameMode() const
{
	return Cast<ADanGameMode>(UGameplayStatics::GetGameMode(this));
}

// =====================================================================
// HUD
// =====================================================================

void UDanHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ADanGameMode* GM = GetDanGameMode();
	if (!GM)
	{
		return;
	}

	const UDanGameParams* P = GM->GetParams();
	const UChargeMinigameBase* Mini = GM->GetChargeMinigame();
	const EDanTrialState S = GM->GetState();

	// 進行中はミニゲームの現在値、確定後は保存された値を見せる
	const float ShownCharge = (S == EDanTrialState::Charging && Mini)
		? Mini->GetChargeAmount()
		: GM->GetCurrentCharge();

	if (MinigameText)
	{
		MinigameText->SetText(Mini ? Mini->GetDisplayName() : FText::GetEmpty());
	}

	if (DanText && P)
	{
		const bool bShow = (S == EDanTrialState::Charging
			|| S == EDanTrialState::ChargeResult
			|| S == EDanTrialState::Firing
			|| S == EDanTrialState::Selecting
			|| S == EDanTrialState::Returning);
		DanText->SetText(bShow
			? FText::FromString(P->GetDanText(ShownCharge))
			: FText::GetEmpty());
	}

	if (ChargeBar)
	{
		ChargeBar->SetPercent(ShownCharge);
	}

	if (ScoreText)
	{
		ScoreText->SetText(FText::FromString(
			FString::Printf(TEXT("かっこよさ度  %s"), *GM->GetScoreText())));
	}

	if (TrialText && P)
	{
		TrialText->SetText(FText::FromString(FString::Printf(
			TEXT("%d / %d"), GM->GetTrialIndex() + 1, P->TrialCount)));
	}

	if (GuideText)
	{
		FString Guide;
		switch (S)
		{
		case EDanTrialState::ChargeIntro:
			Guide = Mini
				? FString::Printf(TEXT("― %s ―"), *Mini->GetDisplayName().ToString())
				: TEXT("");
			break;
		case EDanTrialState::Charging:
			Guide = TEXT("エネルギーを溜めろ");
			break;
		case EDanTrialState::ChargeResult:
			Guide = (Mini && Mini->IsBurst()) ? TEXT("暴発！") : TEXT("");
			break;
		case EDanTrialState::Firing:
			Guide = TEXT("放て");
			break;
		case EDanTrialState::Returning:
			Guide = TEXT("来るぞ");
			break;
		default:
			Guide = TEXT("");
			break;
		}
		GuideText->SetText(FText::FromString(Guide));
	}
}

// =====================================================================
// 防衛の選択
// =====================================================================

void UDanSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SlashButton)
	{
		SlashButton->OnClicked.AddUniqueDynamic(this, &UDanSelectWidget::OnSlashClicked);
	}
	if (CounterButton)
	{
		CounterButton->OnClicked.AddUniqueDynamic(this, &UDanSelectWidget::OnCounterClicked);
	}
}

void UDanSelectWidget::OnSlashClicked()
{
	if (ADanGameMode* GM = GetDanGameMode())
	{
		GM->SelectDefense(EDefenseType::Slash);
	}
}

void UDanSelectWidget::OnCounterClicked()
{
	if (ADanGameMode* GM = GetDanGameMode())
	{
		GM->SelectDefense(EDefenseType::CounterShot);
	}
}

void UDanSelectWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ADanGameMode* GM = GetDanGameMode();
	if (!GM)
	{
		return;
	}

	if (TimerText)
	{
		TimerText->SetText(FText::FromString(
			FString::Printf(TEXT("%.1f"), GM->GetSelectRemaining())));
	}

	if (PromptText)
	{
		const UDanGameParams* P = GM->GetParams();
		const FString Dan = P ? P->GetDanText(GM->GetCurrentCharge()) : TEXT("");
		PromptText->SetText(FText::FromString(
			FString::Printf(TEXT("%s。どう迎え撃つ？"), *Dan)));
	}
}

// =====================================================================
// リザルト
// =====================================================================

void UDanResultWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RetryButton)
	{
		RetryButton->OnClicked.AddUniqueDynamic(this, &UDanResultWidget::OnRetryClicked);
	}
	if (TitleButton)
	{
		TitleButton->OnClicked.AddUniqueDynamic(this, &UDanResultWidget::OnTitleClicked);
	}

	Refresh();
}

void UDanResultWidget::Refresh()
{
	ADanGameMode* GM = GetDanGameMode();
	if (!GM)
	{
		return;
	}

	if (ScoreText)
	{
		ScoreText->SetText(FText::FromString(GM->GetScoreText()));
	}

	if (RankText)
	{
		RankText->SetText(GM->WasRankUpdated()
			? FText::FromString(TEXT("記録更新"))
			: FText::GetEmpty());
	}

	if (DetailText)
	{
		const UDanGameParams* P = GM->GetParams();
		FString Detail;
		int32 i = 1;
		for (const FDanTrialRecord& R : GM->GetRecords())
		{
			FString ResultName;
			switch (R.Result)
			{
			case EDefenseResult::Success:
				ResultName = (R.Accuracy >= 0.90f) ? TEXT("一閃")
					: (R.Accuracy >= 0.70f) ? TEXT("会心")
					: (R.Accuracy >= 0.40f) ? TEXT("有効")
					: TEXT("かすり");
				break;
			case EDefenseResult::Failed:       ResultName = TEXT("被弾"); break;
			case EDefenseResult::Misfire:      ResultName = TEXT("不発"); break;
			case EDefenseResult::Overfilled:   ResultName = TEXT("暴発"); break;
			case EDefenseResult::Unmeasurable: ResultName = TEXT("測定不能"); break;
			default:                           ResultName = TEXT("―"); break;
			}

			const FString DanStr = P ? P->GetDanText(R.Charge) : TEXT("");
			Detail += FString::Printf(TEXT("%d.  %-4s  %-8s  %+d\n"),
				i++, *DanStr, *ResultName, R.Score);
		}
		DetailText->SetText(FText::FromString(Detail));
	}
}

void UDanResultWidget::OnRetryClicked()
{
	if (ADanGameMode* GM = GetDanGameMode())
	{
		GM->RestartGame();
	}
}

void UDanResultWidget::OnTitleClicked()
{
	UGameplayStatics::OpenLevel(this, TEXT("L_Title"));
}

// =====================================================================
// タイトル
// =====================================================================

void UDanTitleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (StartButton)
	{
		StartButton->OnClicked.AddUniqueDynamic(this, &UDanTitleWidget::OnStartClicked);
	}
	if (RankingButton)
	{
		RankingButton->OnClicked.AddUniqueDynamic(this, &UDanTitleWidget::OnRankingClicked);
	}
	if (HowToButton)
	{
		HowToButton->OnClicked.AddUniqueDynamic(this, &UDanTitleWidget::OnHowToClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &UDanTitleWidget::OnQuitClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &UDanTitleWidget::OnBackClicked);
	}

	if (HowToText)
	{
		HowToText->SetText(FText::FromString(
			TEXT("エネルギーを貯めて放ち、それを斬るか撃ち返す。\n")
			TEXT("どれだけかっこよかったかを競え。")));
	}

	BuildRankingText();

	if (Switcher)
	{
		Switcher->SetActiveWidgetIndex(0);
	}
}

void UDanTitleWidget::BuildRankingText()
{
	if (!RankingText)
	{
		return;
	}

	UDanSaveGame* Save = UDanSaveGame::LoadOrCreate();
	FString Body;
	static const TCHAR* Rank[] = { TEXT("１位"), TEXT("２位"), TEXT("３位") };

	for (int32 i = 0; i < UDanSaveGame::MaxEntries; ++i)
	{
		if (Save && Save->TopScores.IsValidIndex(i))
		{
			const FDanRankEntry& E = Save->TopScores[i];
			const int32 Abs = FMath::Abs(E.Score);
			const TCHAR* Sign = (E.Score < 0) ? TEXT("-") : TEXT("");
			Body += FString::Printf(TEXT("%s    %s%07d %%    %s\n"),
				Rank[i], Sign, Abs, *E.GetDateText());
		}
		else
		{
			Body += FString::Printf(TEXT("%s    -------      ----/--/--\n"), Rank[i]);
		}
	}

	RankingText->SetText(FText::FromString(Body));
}

void UDanTitleWidget::OnStartClicked()
{
	UGameplayStatics::OpenLevel(this, GameLevelName);
}

void UDanTitleWidget::OnRankingClicked()
{
	BuildRankingText();
	if (Switcher)
	{
		Switcher->SetActiveWidgetIndex(1);
	}
}

void UDanTitleWidget::OnHowToClicked()
{
	if (Switcher)
	{
		Switcher->SetActiveWidgetIndex(2);
	}
}

void UDanTitleWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(),
		EQuitPreference::Quit, false);
}

void UDanTitleWidget::OnBackClicked()
{
	if (Switcher)
	{
		Switcher->SetActiveWidgetIndex(0);
	}
}
