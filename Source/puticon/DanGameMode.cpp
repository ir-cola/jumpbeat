#include "DanGameMode.h"
#include "DanGameParams.h"
#include "DanSaveGame.h"
#include "DanEnergyBall.h"
#include "ChargeMinigameBase.h"
#include "DefenseModeBase.h"
#include "DanWidgets.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"

ADanGameMode::ADanGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ADanGameMode::BeginPlay()
{
	Super::BeginPlay();

	TrialIndex = 0;
	TotalScore = 0;
	bRankUpdated = false;
	Records.Empty();

	// 全ミニゲーム・全防衛モードにパラメータを配る
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		TArray<UChargeMinigameBase*> Minis;
		PC->GetComponents<UChargeMinigameBase>(Minis);
		for (UChargeMinigameBase* M : Minis)
		{
			M->SetParams(Params);
			M->SetActive(false);
		}

		TArray<UDefenseModeBase*> Defs;
		PC->GetComponents<UDefenseModeBase>(Defs);
		for (UDefenseModeBase* D : Defs)
		{
			D->SetParams(Params);
		}
	}

	// HUD を出す
	if (HUDWidgetClass)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			HUDWidget = CreateWidget<UUserWidget>(PC, HUDWidgetClass);
			if (HUDWidget)
			{
				HUDWidget->AddToViewport(0);
			}
		}
	}

	BeginTrial();
}

void ADanGameMode::UpdateWidgets()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return;
	}

	// --- 防衛の選択UI ---
	const bool bWantSelect = (State == EDanTrialState::Selecting);
	if (bWantSelect && !SelectWidget && SelectWidgetClass)
	{
		SelectWidget = CreateWidget<UUserWidget>(PC, SelectWidgetClass);
		if (SelectWidget)
		{
			SelectWidget->AddToViewport(10);
			// 選択中だけマウスを使えるようにする
			UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(
				PC, SelectWidget, EMouseLockMode::DoNotLock, false);
			PC->bShowMouseCursor = true;
		}
	}
	else if (!bWantSelect && SelectWidget)
	{
		SelectWidget->RemoveFromParent();
		SelectWidget = nullptr;

		// 選択が終わったら操作をゲームに戻す
		if (State != EDanTrialState::Result)
		{
			UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
			PC->bShowMouseCursor = false;
		}
	}

	// --- リザルトUI ---
	const bool bWantResult = (State == EDanTrialState::Result);
	if (bWantResult && !ResultWidget && ResultWidgetClass)
	{
		ResultWidget = CreateWidget<UUserWidget>(PC, ResultWidgetClass);
		if (ResultWidget)
		{
			ResultWidget->AddToViewport(20);
			UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(
				PC, ResultWidget, EMouseLockMode::DoNotLock);
			PC->bShowMouseCursor = true;
		}
	}
	else if (!bWantResult && ResultWidget)
	{
		ResultWidget->RemoveFromParent();
		ResultWidget = nullptr;
		UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
		PC->bShowMouseCursor = false;
	}
}

void ADanGameMode::BeginTrial()
{
	CurrentCharge = 0.f;
	SelectedDefense = EDefenseType::Slash;
	ActiveMinigame = Params
		? Params->GetMinigameForTrial(TrialIndex)
		: EChargeMinigameType::Hold;

	GotoState(EDanTrialState::ChargeIntro);
}

void ADanGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 選択フェーズはスローになるので、実時間で数える
	StateTime += (State == EDanTrialState::Selecting)
		? DeltaSeconds / FMath::Max(KINDA_SMALL_NUMBER, SelectTimeDilation)
		: DeltaSeconds;

	UpdateState(DeltaSeconds);

	if (bShowDebug)
	{
		DrawDebug();
	}
}

void ADanGameMode::GotoState(EDanTrialState NewState)
{
	// 選択フェーズを抜けるときは時間を戻す
	if (State == EDanTrialState::Selecting && NewState != EDanTrialState::Selecting)
	{
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
	}

	State = NewState;
	StateTime = 0.f;

	switch (State)
	{
	case EDanTrialState::Charging:
		if (UChargeMinigameBase* Mini = GetChargeMinigame())
		{
			Mini->SetActive(true);
			Mini->BeginMinigame();
		}
		break;

	case EDanTrialState::Firing:
	{
		// 弾を出して飛ばす
		if (EnergyBallClass && !EnergyBall)
		{
			FVector SpawnLoc = FVector::ZeroVector;
			FRotator SpawnRot = FRotator::ZeroRotator;
			if (APawn* P = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
			{
				SpawnLoc = P->GetActorLocation() + P->GetActorForwardVector() * 150.f + FVector(0, 0, 80.f);
				SpawnRot = P->GetActorRotation();
			}
			FActorSpawnParameters SP;
			SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			EnergyBall = GetWorld()->SpawnActor<ADanEnergyBall>(EnergyBallClass, SpawnLoc, SpawnRot, SP);
		}
		if (EnergyBall)
		{
			const float FlyTime = Params ? Params->TimeFiring : 1.5f;
			EnergyBall->Fire(EnergyBall->GetActorLocation(), CurrentCharge, FlyTime);
		}
		break;
	}

	case EDanTrialState::Selecting:
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), SelectTimeDilation);
		break;

	case EDanTrialState::Returning:
	{
		const float ReturnTime = Params ? Params->GetReturnTime(CurrentCharge) : 3.f;
		PerfectTime = GetWorld()->GetTimeSeconds() + ReturnTime;

		if (UDefenseModeBase* Def = GetDefenseMode())
		{
			Def->BeginDefense(CurrentCharge, PerfectTime);
		}
		if (EnergyBall)
		{
			EnergyBall->BeginReturn(ReturnTime);
		}
		break;
	}

	case EDanTrialState::Resolving:
		if (EnergyBall)
		{
			EnergyBall->Vanish();
		}
		break;

	case EDanTrialState::Result:
	{
		// 棒立ちを含むプレイはランキングに登録しない（設計書 11-2）
		if (!IsUnmeasurable())
		{
			bRankUpdated = UDanSaveGame::SubmitScore(TotalScore);
		}
		break;
	}

	default:
		break;
	}

	UpdateWidgets();
	OnStateEntered(State);
}

void ADanGameMode::UpdateState(float DeltaSeconds)
{
	if (!Params)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();

	switch (State)
	{
	case EDanTrialState::Idle:
		break;

	case EDanTrialState::ChargeIntro:
		if (StateTime >= Params->TimeChargeIntro)
		{
			GotoState(EDanTrialState::Charging);
		}
		break;

	case EDanTrialState::Charging:
		if (UChargeMinigameBase* Mini = GetChargeMinigame())
		{
			if (Mini->IsFinished())
			{
				CurrentCharge = Mini->GetChargeAmount();
				Mini->SetActive(false);
				GotoState(EDanTrialState::ChargeResult);
			}
		}
		break;

	case EDanTrialState::ChargeResult:
		if (StateTime >= Params->TimeChargeResult)
		{
			// 暴発していたら発射も防衛もせず、そのまま判定演出へ
			const UChargeMinigameBase* Mini = GetChargeMinigame();
			if (Mini && Mini->IsBurst())
			{
				GotoState(EDanTrialState::Resolving);
			}
			else
			{
				GotoState(EDanTrialState::Firing);
			}
		}
		break;

	case EDanTrialState::Firing:
		if (StateTime >= Params->TimeFiring)
		{
			GotoState(EDanTrialState::Selecting);
		}
		break;

	case EDanTrialState::Selecting:
		// 何も選ばずに時間切れ → 隠しの「棒立ち」（設計書 4-4）
		if (StateTime >= Params->TimeSelecting)
		{
			SelectDefense(EDefenseType::StandStill);
		}
		break;

	case EDanTrialState::Returning:
	{
		UDefenseModeBase* Def = GetDefenseMode();
		if (!Def)
		{
			GotoState(EDanTrialState::Resolving);
			break;
		}

		// 着弾したら未入力を失敗として確定させる
		if (Now >= PerfectTime && !Def->IsResolved())
		{
			Def->ResolveAtImpact(Now);
		}

		if (Def->IsResolved())
		{
			GotoState(EDanTrialState::Resolving);
		}
		break;
	}

	case EDanTrialState::Resolving:
		if (StateTime >= Params->TimeResolving)
		{
			CommitTrialScore();
			GotoState(EDanTrialState::TrialEnd);
		}
		break;

	case EDanTrialState::TrialEnd:
		if (StateTime >= Params->TimeTrialEnd)
		{
			++TrialIndex;
			if (TrialIndex >= Params->TrialCount)
			{
				GotoState(EDanTrialState::Result);
			}
			else
			{
				BeginTrial();
			}
		}
		break;

	case EDanTrialState::Result:
		// BP側のUIで「もう一度」「タイトルへ」を出す
		break;

	default:
		break;
	}
}

void ADanGameMode::SelectDefense(EDefenseType Type)
{
	if (State != EDanTrialState::Selecting)
	{
		return;
	}

	SelectedDefense = Type;
	GotoState(EDanTrialState::Returning);
}

float ADanGameMode::GetSelectRemaining() const
{
	if (State != EDanTrialState::Selecting || !Params)
	{
		return 0.f;
	}
	return FMath::Max(0.f, Params->TimeSelecting - StateTime);
}

void ADanGameMode::CommitTrialScore()
{
	if (!Params)
	{
		return;
	}

	const UDefenseModeBase* Def = GetDefenseMode();
	const UChargeMinigameBase* Mini = GetChargeMinigame();

	FDanTrialRecord Rec;
	Rec.Charge   = CurrentCharge;
	Rec.Dan      = Params->GetDan(CurrentCharge);
	Rec.Accuracy = Def ? Def->GetAccuracy() : 0.f;
	Rec.Result   = Def ? Def->GetResult() : EDefenseResult::Failed;

	// チャージ段階で暴発していたら、防衛の結果より優先する
	if (Mini && Mini->IsBurst())
	{
		Rec.Result = EDefenseResult::Overfilled;
		Rec.Accuracy = 0.f;
	}

	// 設計書5章
	//   成功: score = ScoreBase * c^ScorePow * a
	//   失敗: score = -ScoreBase * c * FailRate
	//   暴発: score = -ScoreBase * BurstRate
	//   測定不能: 0（合計にも足さない）
	switch (Rec.Result)
	{
	case EDefenseResult::Success:
		Rec.Score = FMath::RoundToInt(
			Params->ScoreBase
			* FMath::Pow(Rec.Charge, Params->ScorePow)
			* Rec.Accuracy);
		break;

	case EDefenseResult::Failed:
	case EDefenseResult::Misfire:
		Rec.Score = -FMath::RoundToInt(
			Params->ScoreBase * Rec.Charge * Params->FailRate);
		break;

	case EDefenseResult::Overfilled:
		Rec.Score = -FMath::RoundToInt(Params->ScoreBase * Params->BurstRate);
		break;

	case EDefenseResult::Unmeasurable:
	default:
		Rec.Score = 0;
		break;
	}

	Records.Add(Rec);

	if (Rec.Result != EDefenseResult::Unmeasurable)
	{
		TotalScore += Rec.Score;
	}
}

bool ADanGameMode::IsUnmeasurable() const
{
	for (const FDanTrialRecord& R : Records)
	{
		if (R.Result == EDefenseResult::Unmeasurable)
		{
			return true;
		}
	}
	return false;
}

FString ADanGameMode::GetScoreText() const
{
	if (IsUnmeasurable())
	{
		return TEXT("測定不能");
	}

	// 設計書5章: 7桁ゼロ埋め＋%
	const int32 Abs = FMath::Abs(TotalScore);
	const TCHAR* Sign = (TotalScore < 0) ? TEXT("-") : TEXT("");
	return FString::Printf(TEXT("%s%07d %%"), Sign, Abs);
}

UChargeMinigameBase* ADanGameMode::GetChargeMinigame() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return nullptr;
	}

	TArray<UChargeMinigameBase*> Minis;
	PC->GetComponents<UChargeMinigameBase>(Minis);
	for (UChargeMinigameBase* M : Minis)
	{
		if (M->GetMinigameType() == ActiveMinigame)
		{
			return M;
		}
	}
	// 該当が無ければ最初の1つで代用する
	return Minis.Num() > 0 ? Minis[0] : nullptr;
}

UDefenseModeBase* ADanGameMode::GetDefenseMode() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return nullptr;
	}

	TArray<UDefenseModeBase*> Defs;
	PC->GetComponents<UDefenseModeBase>(Defs);
	for (UDefenseModeBase* D : Defs)
	{
		if (D->GetDefenseType() == SelectedDefense)
		{
			return D;
		}
	}
	return Defs.Num() > 0 ? Defs[0] : nullptr;
}

void ADanGameMode::RestartGame()
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);

	if (EnergyBall)
	{
		EnergyBall->Destroy();
		EnergyBall = nullptr;
	}

	TrialIndex = 0;
	TotalScore = 0;
	bRankUpdated = false;
	Records.Empty();

	BeginTrial();
}

void ADanGameMode::HandleActionPressed()
{
	const float Now = GetWorld()->GetTimeSeconds();

	switch (State)
	{
	case EDanTrialState::Charging:
		if (UChargeMinigameBase* Mini = GetChargeMinigame())
		{
			Mini->OnPressStart(Now);
		}
		break;

	case EDanTrialState::Returning:
		if (UDefenseModeBase* Def = GetDefenseMode())
		{
			Def->OnInputPressed(Now);
		}
		break;

	case EDanTrialState::Result:
		RestartGame();
		break;

	default:
		break;
	}
}

void ADanGameMode::HandleActionReleased()
{
	const float Now = GetWorld()->GetTimeSeconds();

	switch (State)
	{
	case EDanTrialState::Charging:
		if (UChargeMinigameBase* Mini = GetChargeMinigame())
		{
			Mini->OnPressEnd(Now);
		}
		break;

	case EDanTrialState::Returning:
		if (UDefenseModeBase* Def = GetDefenseMode())
		{
			Def->OnInputReleased(Now);
		}
		break;

	default:
		break;
	}
}

void ADanGameMode::HandlePointerMoved(FVector2D Delta)
{
	if (State == EDanTrialState::Charging)
	{
		if (UChargeMinigameBase* Mini = GetChargeMinigame())
		{
			Mini->OnPointerMove(Delta);
		}
	}
}

void ADanGameMode::HandleClickAt(FVector2D ScreenPos)
{
	if (State == EDanTrialState::Charging)
	{
		if (UChargeMinigameBase* Mini = GetChargeMinigame())
		{
			Mini->OnClickAt(ScreenPos);
		}
	}
}

void ADanGameMode::DrawDebug() const
{
	if (!GEngine)
	{
		return;
	}

	const UEnum* StateEnum = StaticEnum<EDanTrialState>();
	const FString StateName = StateEnum
		? StateEnum->GetDisplayNameTextByValue((int64)State).ToString()
		: TEXT("?");

	float MiniCharge = 0.f;
	bool  bBurst = false;
	FString MiniName = TEXT("-");
	if (const UChargeMinigameBase* Mini = GetChargeMinigame())
	{
		MiniCharge = Mini->GetChargeAmount();
		bBurst = Mini->IsBurst();
		MiniName = Mini->GetDisplayName().ToString();
	}

	const FString Line1 = FString::Printf(
		TEXT("[%d/%d] %s  t=%.2f  (%s)"),
		TrialIndex + 1,
		Params ? Params->TrialCount : 3,
		*StateName,
		StateTime,
		*MiniName);

	const FString Line2 = FString::Printf(
		TEXT("c=%.3f %s%s   Score=%d"),
		MiniCharge,
		Params ? *Params->GetDanText(MiniCharge) : TEXT(""),
		bBurst ? TEXT(" [暴発]") : TEXT(""),
		TotalScore);

	GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Yellow, Line1);
	GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Cyan, Line2);

	if (State == EDanTrialState::Returning)
	{
		const float Left = PerfectTime - GetWorld()->GetTimeSeconds();
		const FString Line3 = FString::Printf(
			TEXT("着弾まで %.2f 秒   判定窓 ±%.3f 秒   防衛=%s"),
			Left,
			Params ? Params->GetWindow(CurrentCharge) : 0.f,
			GetDefenseMode() ? *GetDefenseMode()->GetDisplayName().ToString() : TEXT("-"));
		GEngine->AddOnScreenDebugMessage(3, 0.f, FColor::Green, Line3);
	}
}
