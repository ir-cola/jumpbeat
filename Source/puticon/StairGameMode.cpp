#include "StairGameMode.h"
#include "StairStep.h"
#include "StairCharacter.h"
#include "StairPlayerController.h"
#include "StairConfig.h"
#include "StairTerrain.h"
#include "StairMusicClock.h"
#include "StairWidgets.h"
#include "StairGameInstance.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "StairSpark.h"
#include "StairMeteor.h"
#include "StairBullet.h"
#include "Sound/SoundBase.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

AStairGameMode::AStairGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultPawnClass = AStairCharacter::StaticClass();
	PlayerControllerClass = AStairPlayerController::StaticClass();

	HUDWidgetClass = UStairHUDWidget::StaticClass();
	ResultWidgetClass = UStairResultWidget::StaticClass();

	MusicClock = CreateDefaultSubobject<UStairMusicClock>(TEXT("MusicClock"));
	Terrain = CreateDefaultSubobject<UStairTerrain>(TEXT("Terrain"));
}

void AStairGameMode::BeginPlay()
{
	Super::BeginPlay();

	Score = 0;
	PerfectCount = GreatCount = MissCount = 0;
	JumpMissCount = ShotMissCount = 0;
	Combo = MaxCombo = 0;
	NextMeteorIndex = 0;
	ReloadLeft = 0.f;
	Meteors.Reset();
	Charges = Config ? Config->MagazineSize : 3;
	EndReason = EStairEndReason::None;

	CacheSceneActors();

	// ---- 選ばれた曲を取り出す ----
	int32 SongIndex = 0;
	if (UStairGameInstance* GI = Cast<UStairGameInstance>(GetGameInstance()))
	{
		SongIndex = GI->SelectedSongIndex;
	}
	if (Config && Config->Songs.IsValidIndex(SongIndex))
	{
		CurrentSong = Config->Songs[SongIndex];
	}

	// ---- 地形 ----
	if (Terrain)
	{
		Terrain->Initialize(Config);
		if (!Terrain->StepClass)
		{
			Terrain->StepClass = AStairStep::StaticClass();
		}
		Terrain->UpdateAround(0, 0, 0.f);
	}

	// ---- プレイヤー ----
	// ★ここで取れないことがある。取れるまで Tick で試し続ける
	bPlayerPlaced = false;
	TryInitPlayer();

	// ★前のレベル（BGM選択やリザルト）で UIOnly にした入力モードを必ず戻す。
	//   これを忘れると、曲選択から入り直したときにキー入力が届かなくなる。
	RestoreGameInput();

	// ---- 曲を鳴らす。カウントダウンぶん遅らせる ----
	const float Countdown = Config ? Config->CountdownSeconds : 3.2f;
	if (MusicClock)
	{
		// 環境ごとの音の遅れ補正を渡す
		MusicClock->SetGlobalOffset(Config ? Config->AudioOffsetMs * 0.001f : 0.f);
		// ★ゲージの速さ。2 拍で1回の判定にすると連続で跳べる
		MusicClock->SetBeatScale(Config ? Config->BeatsPerJudge : 2.f);

		// ★まずテンポ合わせ。曲は鳴らさず、時計だけ拍を刻ませる。
		//   ここで拍を掴んでもらってから、カウントダウン→曲へ進む。
		MusicClock->StartSong(nullptr, CurrentSong.BPM, 0.f, 0.f);
	}

	IntroTaps = 0;
	IntroLastBeat = -9999;
	IntroLastGuideBeat = -9999;

	SetState(EStairGameState::Intro);
}

AStairCharacter* AStairGameMode::GetPlayerChar() const
{
	// ★毎回取り直す。BeginPlay 時点では Pawn が未生成のことがある
	if (!Player)
	{
		Player = Cast<AStairCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	}
	return Player;
}

void AStairGameMode::RestoreGameInput()
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
		PC->bShowMouseCursor = false;
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
		PC->EnableInput(PC);
	}
}

void AStairGameMode::TryInitPlayer()
{
	if (bPlayerPlaced)
	{
		return;
	}

	AStairCharacter* P = GetPlayerChar();
	if (!P || !Terrain)
	{
		return;
	}

	P->SetControlEnabled(State == EStairGameState::Playing);
	P->PlaceOnStep(0, 0, Terrain->GetStepLocation(0, 0));
	bPlayerPlaced = true;
}

void AStairGameMode::UpdateTimingTuner()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !MusicClock)
	{
		return;
	}

	// 入力アセットを増やさずに済むよう、生のキーを直接見る
	float Delta = 0.f;
	bool bReset = false;

	if (PC->WasInputKeyJustPressed(EKeys::LeftBracket))   { Delta = -5.f; }
	if (PC->WasInputKeyJustPressed(EKeys::RightBracket))  { Delta = 5.f; }
	if (PC->WasInputKeyJustPressed(EKeys::Backslash))     { bReset = true; }

	if (bReset)
	{
		MusicClock->SetGlobalOffset(0.f);
		TunerShowTime = 3.f;
	}
	else if (Delta != 0.f)
	{
		MusicClock->NudgeGlobalOffsetMs(Delta);
		TunerShowTime = 3.f;
	}
}

FString AStairGameMode::GetTimingTunerText() const
{
	if (TunerShowTime <= 0.f || !MusicClock)
	{
		return FString();
	}
	return FString::Printf(
		TEXT("タイミング補正  %+.0f ms      [ ] で調整   \\ でリセット"),
		MusicClock->GetGlobalOffsetMs());
}

void AStairGameMode::PlaySystemSound(USoundBase* Sound)
{
	if (Sound && Config)
	{
		UGameplayStatics::PlaySound2D(this, Sound, Config->SystemSoundVolume);
	}
}

void AStairGameMode::UpdateCountdownSound()
{
	if (State != EStairGameState::Countdown || !MusicClock || !Config)
	{
		return;
	}

	const int32 Num = GetCountdownNumber();

	// 3 → 2 → 1 と変わるたびに鳴らす。間隔はきっちり1秒
	if (Num >= 1 && Num <= 3 && Num != LastCountdownNumber)
	{
		LastCountdownNumber = Num;
		PlaySystemSound(Config->CountdownSound);
	}
}

int32 AStairGameMode::GetCountdownNumber() const
{
	if (!MusicClock)
	{
		return -1;
	}

	const float Left = MusicClock->GetCountdownRemaining();
	if (Left <= 0.f)
	{
		return 0;               // START
	}

	// ★1秒刻みではなく「拍」で数える。
	//   メーターが上端を通るのと同じ間隔で 3・2・1 が入るので、
	//   曲に入る瞬間のテンポが体で分かる。
	const float Beat = FMath::Max(0.05f, MusicClock->GetBeatDuration());
	const int32 N = FMath::CeilToInt(Left / Beat);

	return (N <= 3) ? N : -1;
}

void AStairGameMode::CacheSceneActors()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		SunLight = *It;
		break;
	}
	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
	{
		Fog = *It;
		break;
	}
	for (TActorIterator<ASkyLight> It(World); It; ++It)
	{
		SkyLightActor = *It;
		break;
	}
}

// =====================================================================
// 曲と進行度
// =====================================================================

float AStairGameMode::GetT() const
{
	// ★テンポ合わせの最中は曲が鳴っていない。
	//   ここで時計を進めると、曲が始まる前から残り時間が減ってしまう。
	if (State == EStairGameState::Intro)
	{
		return 0.f;
	}
	return MusicClock ? MusicClock->GetT() : 0.f;
}

float AStairGameMode::GetRemainingTime() const
{
	if (!MusicClock)
	{
		return 0.f;
	}

	// ★曲が鳴り始めるまでは満タンのまま止めておく。
	//   テンポ合わせ中に残り時間が減っていくのは不自然なため。
	if (State == EStairGameState::Intro)
	{
		return MusicClock->GetSongLength();
	}

	return FMath::Max(0.f,
		MusicClock->GetSongLength() - FMath::Max(0.f, MusicClock->GetSongTime()));
}

FString AStairGameMode::GetCountdownText() const
{
	if (State != EStairGameState::Countdown || !MusicClock)
	{
		return FString();
	}

	// ★音と同じ関数から作る。表示と音がずれないようにするため
	const int32 Num = GetCountdownNumber();
	if (Num < 0)  { return FString(); }   // まだ数え始めていない
	if (Num == 0) { return TEXT("START"); }
	return FString::FromInt(Num);
}

// =====================================================================
// リズム判定。★オーディオ再生位置が基準
// =====================================================================

EStairJudge AStairGameMode::JudgeNow() const
{
	if (!MusicClock || !Config)
	{
		return EStairJudge::Miss;
	}

	// 拍からのズレ（秒）→ ミリ秒
	const float OffsetMs = FMath::Abs(MusicClock->GetOffsetFromNearestBeat()) * 1000.f;

	if (OffsetMs <= Config->PerfectWindowMs)
	{
		return EStairJudge::Perfect;
	}
	if (OffsetMs <= Config->GreatWindowMs)
	{
		return EStairJudge::Great;
	}
	return EStairJudge::Miss;
}

float AStairGameMode::GetBeatPhase() const
{
	return MusicClock ? MusicClock->GetBeatPhase() : 0.f;
}

float AStairGameMode::GetPerfectZoneHalfWidth() const
{
	if (!MusicClock || !Config)
	{
		return 0.f;
	}
	const float Beat = MusicClock->GetBeatDuration();
	return (Beat > 0.f) ? ((Config->PerfectWindowMs * 0.001f) / Beat) : 0.f;
}

float AStairGameMode::GetGreatZoneHalfWidth() const
{
	if (!MusicClock || !Config)
	{
		return 0.f;
	}
	const float Beat = MusicClock->GetBeatDuration();
	return (Beat > 0.f) ? ((Config->GreatWindowMs * 0.001f) / Beat) : 0.f;
}

// =====================================================================
// 進行
// =====================================================================

void AStairGameMode::SetState(EStairGameState NewState)
{
	State = NewState;
	StateTime = 0.f;

	// ★Result中は入力を完全に遮断する。Pawn は毎回取り直す
	if (AStairCharacter* P = GetPlayerChar())
	{
		P->SetControlEnabled(State == EStairGameState::Playing);
	}

	// Playing に入るときは入力モードを必ずゲームに戻す
	if (State == EStairGameState::Playing)
	{
		RestoreGameInput();
	}

	UpdateWidgets();
	OnStateChanged(State);
}

void AStairGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	StateTime += DeltaSeconds;

	// Pawn がまだ取れていなければ取れるまで試す
	TryInitPlayer();

	// タイミング補正を耳で合わせるための操作を受け付ける
	UpdateTimingTuner();

	// 譜面編集の操作
	UpdateCharting();
	TunerShowTime = FMath::Max(0.f, TunerShowTime - DeltaSeconds);

	// t は1箇所でだけ求め、全システムに配る
	const float T = GetT();
	UpdateSkyByT(T);

	switch (State)
	{
	case EStairGameState::Intro:
		UpdateIntro(DeltaSeconds);
		break;

	case EStairGameState::Countdown:
		UpdateCountdownSound();
		if (MusicClock && MusicClock->HasStarted())
		{
			SetState(EStairGameState::Playing);
		}
		break;

	case EStairGameState::Playing:
	{
		AStairCharacter* P = GetPlayerChar();
		if (P && Terrain)
		{
			Terrain->UpdateAround(P->GetCurrentRow(), P->GetCurrentLane(), T);
		}

		// 保険: 何らかの理由で操作が無効なままなら有効に戻す
		if (P && !P->IsControlEnabled())
		{
			P->SetControlEnabled(true);
		}

		// リロードを進める
		if (ReloadLeft > 0.f)
		{
			ReloadLeft -= DeltaSeconds;
			if (ReloadLeft <= 0.f)
			{
				ReloadLeft = 0.f;
				Charges = Config ? Config->MagazineSize : 3;
			}
		}

		// 譜面にしたがって隕石を落とす
		UpdateMeteors(DeltaSeconds);

		// 残像とゲージの退場
		UpdateGaugeState(DeltaSeconds);

		if (MusicClock && MusicClock->IsSongFinished())
		{
			EndGame(EStairEndReason::SongEnd);
		}
		break;
	}

	case EStairGameState::Finished:
		if (StateTime >= FinishedHold)
		{
			SetState(EStairGameState::Result);
		}
		break;

	default:
		break;
	}
}

void AStairGameMode::EndGame(EStairEndReason Reason)
{
	if (State == EStairGameState::Finished || State == EStairGameState::Result)
	{
		return;
	}

	EndReason = Reason;

	if (Config)
	{
		if (Reason == EStairEndReason::SongEnd)
		{
			// 死なずに完走した
			PlaySystemSound(Config->ClearSound);
		}
		else
		{
			// 落下・崩落によるゲームオーバー
			PlaySystemSound(Config->ResultSound);
		}
	}

	if (MusicClock)
	{
		MusicClock->StopSong();
	}

	// Playing → Finished → Result
	SetState(EStairGameState::Finished);
}

void AStairGameMode::NotifyLandedOn(AStairStep* Step)
{
	if (State != EStairGameState::Playing || !Step)
	{
		return;
	}
	Score = FMath::Max(Score, Step->Row);
}

void AStairGameMode::NotifyJudge(EStairJudge Judge)
{
	if (State != EStairGameState::Playing)
	{
		return;
	}
	switch (Judge)
	{
	case EStairJudge::Perfect: ++PerfectCount; break;
	case EStairJudge::Great:   ++GreatCount;   break;
	default: break; // MISS は NotifyMissOnCurrentStep で数える
	}

	UpdateCombo(Judge);

	// ★押した位置を残像として覚えておく。
	//   走り気味か遅れ気味かが、ゲージ上で目に見えるようになる。
	if (MusicClock)
	{
		FStairGhost G;
		G.Phase = MusicClock->GetBeatPhase();
		G.Age = 0.f;
		G.Judge = Judge;
		Ghosts.Insert(G, 0);

		const int32 Keep = Config ? FMath::Max(1, Config->GhostCount) : 3;
		if (Ghosts.Num() > Keep)
		{
			Ghosts.SetNum(Keep);
		}
	}
}

// =====================================================================
// 譜面づくり
// =====================================================================

void AStairGameMode::ChartToggle()
{
	bCharting = !bCharting;

	if (bCharting)
	{
		// いま選んでいる曲の譜面を読み込んで、続きから打てるようにする
		ChartBeats = CurrentSong.MeteorBeats;
		UE_LOG(LogTemp, Warning,
			TEXT("[譜面] 編集を開始。既存 %d 個。左クリックで置く / BackSpace で取消 / P で書き出し"),
			ChartBeats.Num());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[譜面] 編集を終了"));
	}
}

void AStairGameMode::ChartPlace()
{
	if (!bCharting || !MusicClock)
	{
		return;
	}

	// ★クリックした時刻を最寄りの拍に丸める。
	//   多少ずれて押しても、譜面としては拍ぴったりになる。
	const int32 Beat = FMath::RoundToInt(MusicClock->GetBeatPosition());

	if (ChartBeats.Contains(Beat))
	{
		return;   // 同じ拍に二重に置かない
	}

	ChartBeats.Add(Beat);
	ChartBeats.Sort();

	PlaySystemSound(Config ? Config->CountdownSound : nullptr);
	UE_LOG(LogTemp, Warning, TEXT("[譜面] 拍 %d に配置（計 %d 個）"),
		Beat, ChartBeats.Num());
}

void AStairGameMode::ChartUndo()
{
	if (!bCharting || ChartBeats.Num() == 0)
	{
		return;
	}

	// 直前に置いたもの＝いまの時刻にいちばん近いものを消す
	int32 Best = 0;
	if (MusicClock)
	{
		const float Now = MusicClock->GetBeatPosition();
		float BestDiff = BIG_NUMBER;
		for (int32 i = 0; i < ChartBeats.Num(); ++i)
		{
			const float D = FMath::Abs(ChartBeats[i] - Now);
			if (D < BestDiff) { BestDiff = D; Best = i; }
		}
	}
	else
	{
		Best = ChartBeats.Num() - 1;
	}

	UE_LOG(LogTemp, Warning, TEXT("[譜面] 拍 %d を取消（残り %d 個）"),
		ChartBeats[Best], ChartBeats.Num() - 1);
	ChartBeats.RemoveAt(Best);
}

void AStairGameMode::ChartExport()
{
	if (ChartBeats.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[譜面] 空なので書き出しません"));
		return;
	}

	FString Line;
	for (int32 i = 0; i < ChartBeats.Num(); ++i)
	{
		Line += FString::FromInt(ChartBeats[i]);
		if (i < ChartBeats.Num() - 1) { Line += TEXT(","); }
	}

	const FString Text = FString::Printf(
		TEXT("曲: %s\nBPM: %.2f\n拍数: %d\nMeteorBeats:\n%s\n"),
		*CurrentSong.Title, CurrentSong.BPM, ChartBeats.Num(), *Line);

	// プロジェクト直下に書き出す。DataAsset へは手で貼るか Python で流し込む
	const FString Path = FPaths::ProjectSavedDir()
		/ TEXT("Chart_") / (CurrentSong.Title + TEXT(".txt"));

	FFileHelper::SaveStringToFile(Text, *Path);

	UE_LOG(LogTemp, Warning, TEXT("[譜面] 書き出しました: %s"), *Path);
	UE_LOG(LogTemp, Warning, TEXT("[譜面] %s"), *Line);
}

FString AStairGameMode::GetChartText() const
{
	if (!bCharting)
	{
		return FString();
	}

	const int32 Beat = MusicClock
		? FMath::RoundToInt(MusicClock->GetBeatPosition()) : 0;

	return FString::Printf(
		TEXT("譜面編集中　拍 %d　置いた数 %d\n")
		TEXT("左クリック=置く　BackSpace=取消　P=書き出し　O=終了"),
		Beat, ChartBeats.Num());
}

void AStairGameMode::UpdateCharting()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	// O で編集モードの出入り
	if (PC->WasInputKeyJustPressed(EKeys::O))
	{
		ChartToggle();
	}

	if (!bCharting)
	{
		return;
	}

	if (PC->WasInputKeyJustPressed(EKeys::BackSpace)) { ChartUndo(); }
	if (PC->WasInputKeyJustPressed(EKeys::P))         { ChartExport(); }
}

// =====================================================================
// イントロ（テンポ合わせ）
// =====================================================================

int32 AStairGameMode::GetIntroNeeded() const
{
	return Config ? FMath::Max(1, Config->IntroTapCount) : 5;
}

FString AStairGameMode::GetIntroText() const
{
	if (State != EStairGameState::Intro)
	{
		return FString();
	}
	return FString::Printf(
		TEXT("テンポに合わせて SPACE を %d回\n%d / %d"),
		GetIntroNeeded(), IntroTaps, GetIntroNeeded());
}

void AStairGameMode::NotifyIntroTap()
{
	if (State != EStairGameState::Intro || !MusicClock || !Config)
	{
		return;
	}

	// ★判定は本編と同じ窓を使う。PERFECT でも GREAT でも成功とする。
	//   始める前に厳しくすると、詰まって先へ進めなくなるため。
	const EStairJudge Judge = JudgeNow();
	const int32 BeatIndex = FMath::RoundToInt(MusicClock->GetBeatPosition());

	if (Judge == EStairJudge::Miss)
	{
		if (Config->bIntroResetOnMiss)
		{
			IntroTaps = 0;
			IntroLastBeat = -9999;
		}
		PlaySystemSound(Config->WarningSound);
		return;
	}

	// 同じ拍で二重に数えない
	if (BeatIndex == IntroLastBeat)
	{
		return;
	}

	IntroLastBeat = BeatIndex;
	++IntroTaps;
	PlaySystemSound(Config->CountdownSound);
}

void AStairGameMode::NotifyPracticeTap()
{
	if (!MusicClock || !Config)
	{
		return;
	}

	// 進みはしない。押した位置だけ残像に残して、拍を確かめられるようにする
	FStairGhost G;
	G.Phase = MusicClock->GetBeatPhase();
	G.Age = 0.f;
	G.Judge = JudgeNow();
	Ghosts.Insert(G, 0);

	const int32 Keep = FMath::Max(1, Config->GhostCount);
	if (Ghosts.Num() > Keep)
	{
		Ghosts.SetNum(Keep);
	}

	// 判定に応じて音を鳴らす。ジャンプ音のピッチ違いを流用する
	if (Config->JumpSound)
	{
		float Pitch = Config->JumpPitchGreat;
		if (G.Judge == EStairJudge::Perfect) { Pitch = Config->JumpPitchPerfect; }
		else if (G.Judge == EStairJudge::Miss) { Pitch = Config->JumpPitchMiss; }

		UGameplayStatics::PlaySound2D(this, Config->JumpSound,
			Config->JumpSoundVolume * 0.6f, Pitch);
	}
}

void AStairGameMode::UpdateIntro(float DeltaSeconds)
{
	if (!MusicClock || !Config)
	{
		return;
	}

	// ---- 拍ごとにガイド音を鳴らす ----
	const float Phase = MusicClock->GetBeatPhase();

	// ★位相が1周して0へ戻った瞬間＝拍のジャスト。
	//   「位相が0.25未満なら鳴らす」だと、拍から最大で1/4拍ぶん遅れて
	//   鳴ってしまい、手拍子がずれて聞こえる。折り返しを見て1フレーム内で鳴らす。
	const bool bCrossed = (Phase < IntroPrevPhase - 0.5f);
	IntroPrevPhase = Phase;

	if (bCrossed)
	{
		if (Config->IntroClapSound)
		{
			UGameplayStatics::PlaySound2D(this, Config->IntroClapSound,
				Config->IntroClapVolume);
		}
		else
		{
			PlaySystemSound(Config->ButtonSound);
		}
	}

	// ---- 押し終わったら、拍に合わせたカウントダウンへ ----
	if (IntroTaps >= GetIntroNeeded())
	{
		// ★カウントダウンも曲のテンポで刻む。
		//   拍の間隔ちょうどで 3・2・1 が入るよう、開始を拍に合わせる。
		const float Beat = MusicClock->GetBeatDuration();
		const float Countdown = Beat * 3.f;

		MusicClock->SetGlobalOffset(Config->AudioOffsetMs * 0.001f);
		MusicClock->SetBeatScale(Config->BeatsPerJudge);
		MusicClock->StartSong(CurrentSong.Sound, CurrentSong.BPM,
			CurrentSong.BeatOffset, Countdown);

		LastCountdownNumber = 0;
		SetState(EStairGameState::Countdown);
	}
}

void AStairGameMode::UpdateGaugeState(float DeltaSeconds)
{
	// ---- 残像を古くしていく ----
	const float Fade = Config ? FMath::Max(0.1f, Config->GhostFadeSeconds) : 1.6f;
	for (int32 i = Ghosts.Num() - 1; i >= 0; --i)
	{
		Ghosts[i].Age += DeltaSeconds;
		if (Ghosts[i].Age > Fade)
		{
			Ghosts.RemoveAt(i);
		}
	}

	// ---- 一定の段数までのぼったらゲージを去らせる ----
	if (GaugeExitAlpha >= 1.f)
	{
		return;   // 一度去ったら戻さない
	}

	const int32 ExitRow = Config ? Config->GaugeExitRow : 50;
	AStairCharacter* P = GetPlayerChar();

	if (GaugeExitAlpha > 0.f || (P && P->GetCurrentRow() >= ExitRow))
	{
		const float Sec = Config ? FMath::Max(0.05f, Config->GaugeExitSeconds) : 0.9f;
		GaugeExitAlpha = FMath::Min(1.f, GaugeExitAlpha + DeltaSeconds / Sec);
	}
}

// =====================================================================
// 射撃
// =====================================================================

float AStairGameMode::GetReloadProgress() const
{
	const float Total = Config ? Config->ReloadSeconds : 2.f;
	if (Total <= 0.f || ReloadLeft <= 0.f)
	{
		return 1.f;
	}
	return FMath::Clamp(1.f - (ReloadLeft / Total), 0.f, 1.f);
}

AStairMeteor* AStairGameMode::FindShotTarget() const
{
	AStairCharacter* P = GetPlayerChar();
	if (!P || !Config)
	{
		return nullptr;
	}

	// ★射程は「足場で言う1段上まで」。早撃ちしても届かない。
	//   遠くの隕石は狙えないので、撃つのはタイミングを合わせる行為になる。
	const float Reach = Config->StepDepth * (Config->BulletRangeRows + 0.75f);
	const FVector Origin = P->GetActorLocation();

	AStairMeteor* Best = nullptr;
	float BestDiff = BIG_NUMBER;

	for (AStairMeteor* M : Meteors)
	{
		if (!M || !M->IsAlive())
		{
			continue;
		}
		if (FVector::Dist(Origin, M->GetActorLocation()) > Reach)
		{
			continue;
		}

		// 判定に最も近いもの（＝いちばん撃ち頃）を選ぶ
		const float Diff = FMath::Abs(1.f - M->GetApproach());
		if (Diff < BestDiff)
		{
			BestDiff = Diff;
			Best = M;
		}
	}
	return Best;
}

void AStairGameMode::FireShot()
{
	// ★譜面編集中は、左クリックが「置く」操作になる
	if (bCharting)
	{
		ChartPlace();
		return;
	}

	if (State != EStairGameState::Playing || !Config)
	{
		return;
	}
	if (ReloadLeft > 0.f)
	{
		return;   // リロード中は撃てない
	}

	AStairCharacter* P = GetPlayerChar();
	UWorld* World = GetWorld();
	if (!P || !World)
	{
		return;
	}

	AStairMeteor* Target = FindShotTarget();
	const EStairJudge Judge = Target ? Target->JudgeShot() : EStairJudge::Miss;

	// ---- 弾を出す ----
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector Muzzle = P->GetActorLocation() + FVector(0.f, 0.f, 40.f);
	if (AStairBullet* B = World->SpawnActor<AStairBullet>(
		AStairBullet::StaticClass(), Muzzle, FRotator::ZeroRotator, SP))
	{
		B->SetBulletScale(Config->BulletScale);
		// 当たる場合だけ誘導する。外したら素通りさせて見た目で分かるようにする
		B->Fire(Judge != EStairJudge::Miss ? Target : nullptr,
			FVector::ForwardVector, Config->BulletSpeed);
	}

	PlaySystemSound(Config->JumpSound);

	// ---- チャージの増減 ----
	if (Judge == EStairJudge::Miss)
	{
		// ★外すとチャージが減る。3回外すとリロード。
		++MissCount;
		++ShotMissCount;
		Combo = 0;
		LastComboBeat = -9999;

		--Charges;
		if (Charges <= 0)
		{
			Charges = 0;
			ReloadLeft = Config->ReloadSeconds;
		}
	}
	else
	{
		// ★当てるとチャージが満タンに戻る。当て続ければ撃ち続けられる
		if (Judge == EStairJudge::Perfect) { ++PerfectCount; }
		else                               { ++GreatCount; }

		Charges = Config->MagazineSize;
	}
}

// =====================================================================
// 隕石
// =====================================================================

void AStairGameMode::UpdateMeteors(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	if (!World || !Config || !MusicClock || !Terrain)
	{
		return;
	}

	// ---- 譜面を見て、猶予ぶん手前で落とし始める ----
	const float Beat = MusicClock->GetBeatDuration();
	const float Now = MusicClock->GetSongTime();

	while (CurrentSong.MeteorBeats.IsValidIndex(NextMeteorIndex))
	{
		const int32 BeatIndex = CurrentSong.MeteorBeats[NextMeteorIndex];
		const float ImpactTime = BeatIndex * Beat + CurrentSong.BeatOffset;

		// 着弾の MeteorLeadSeconds 前に出す
		if (Now < ImpactTime - Config->MeteorLeadSeconds)
		{
			break;
		}
		++NextMeteorIndex;

		AStairCharacter* P = GetPlayerChar();
		if (!P)
		{
			continue;
		}

		// 着弾点はプレイヤーの少し先。斜めに落ちてくる
		const FVector Aim = Terrain->GetStepLocation(
			P->GetCurrentRow() + 1, P->GetCurrentLane()) + FVector(0.f, 0.f, 60.f);

		const FVector SpawnAt = Aim
			+ Config->MeteorSpawnOffset
			+ FVector(0.f, 0.f, Config->MeteorSpawnHeight);

		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AStairMeteor* M = World->SpawnActor<AStairMeteor>(
			AStairMeteor::StaticClass(), SpawnAt, FRotator::ZeroRotator, SP))
		{
			M->SetJudgeWidths(Config->ShotPerfectWidth, Config->ShotGreatWidth);
			M->SetSpin(Config->MeteorSpin);
			M->SetMeteorScale(Config->MeteorScale);
			M->Launch(Aim, Config->MeteorLeadSeconds);
			Meteors.Add(M);
		}
	}

	// ---- 着弾したものを処理し、片付ける ----
	for (int32 i = Meteors.Num() - 1; i >= 0; --i)
	{
		AStairMeteor* M = Meteors[i];
		if (!M || !IsValid(M))
		{
			Meteors.RemoveAt(i);
			continue;
		}
		if (!M->IsAlive())
		{
			Meteors.RemoveAt(i);
			continue;
		}
		if (M->HasImpacted())
		{
			HandleMeteorImpact(M);
			Meteors.RemoveAt(i);
			M->Destroy();
		}
	}
}

void AStairGameMode::HandleMeteorImpact(AStairMeteor* M)
{
	AStairCharacter* P = GetPlayerChar();
	UWorld* World = GetWorld();
	if (!P || !Terrain || !Config || !World)
	{
		return;
	}

	// ★撃ち漏らしは「足元」ではなく「この先の道」を削る。
	//   撃ち漏らすほど進路に穴が増えて、後で避けきれなくなる。
	++MissCount;
	++ShotMissCount;
	Combo = 0;
	LastComboBeat = -9999;

	const int32 PRow = P->GetCurrentRow();
	const int32 PLane = P->GetCurrentLane();
	const int32 Spread = FMath::Max(0, Config->MeteorBreakLaneSpread);

	// 2〜5段先の、足場が実在するマスから選ぶ。画面内で壊れるようにする
	TArray<TPair<int32, int32>> Cands;
	for (int32 dr = Config->MeteorBreakRowMin; dr <= Config->MeteorBreakRowMax; ++dr)
	{
		for (int32 dl = -Spread; dl <= Spread; ++dl)
		{
			const int32 R = PRow + dr;
			const int32 L = PLane + dl;
			if (Terrain->HasLandableStep(R, L))
			{
				Cands.Add(TPair<int32, int32>(R, L));
			}
		}
	}

	if (Cands.Num() == 0)
	{
		return;
	}

	const TPair<int32, int32> Pick = Cands[FMath::RandRange(0, Cands.Num() - 1)];
	const FVector At = Terrain->GetStepLocation(Pick.Key, Pick.Value);

	Terrain->RemoveStep(Pick.Key, Pick.Value);

	// 爆発させる
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AStairSpark* S = World->SpawnActor<AStairSpark>(
		AStairSpark::StaticClass(), At + FVector(0.f, 0.f, 40.f),
		FRotator::ZeroRotator, SP))
	{
		S->Burst(FLinearColor(1.f, 0.42f, 0.06f, 1.f), 2.0f);
	}

	PlaySystemSound(Config->WarningSound);
}

void AStairGameMode::NotifyWallBounce()
{
	// 判定（Miss）は NotifyJudge で通知済み。ここでは耐久だけ減らす
	NotifyMissOnCurrentStep();
}

void AStairGameMode::UpdateCombo(EStairJudge Judge)
{
	// MISS で途切れる
	if (Judge == EStairJudge::Miss)
	{
		Combo = 0;
		LastComboBeat = -9999;
		return;
	}

	// いま何拍目かを整数で取る
	const int32 BeatIndex = MusicClock
		? FMath::RoundToInt(MusicClock->GetBeatPosition()) : 0;

	// ★1つ前の拍から続いていれば伸ばす。
	//   拍を1つでも空けたら（待ったら）そこで切れて1から数え直す。
	if (Combo > 0 && BeatIndex == LastComboBeat + 1)
	{
		++Combo;
	}
	else
	{
		Combo = 1;
	}

	LastComboBeat = BeatIndex;
	MaxCombo = FMath::Max(MaxCombo, Combo);

	const int32 Every = Config ? FMath::Max(1, Config->ComboSparkEvery) : 10;
	if (Combo > 0 && (Combo % Every) == 0)
	{
		SpawnComboSparks();
	}
}

void AStairGameMode::SpawnComboSparks()
{
	AStairCharacter* P = GetPlayerChar();
	UWorld* World = GetWorld();
	if (!P || !Terrain || !World || !Config)
	{
		return;
	}

	const int32 PRow = P->GetCurrentRow();
	const int32 PLane = P->GetCurrentLane();

	// ★プレイヤーが立っている足場の左右に固定して打ち上げる。
	//   遠くのランダムな位置だと、自分の手柄という感じが出ないため。
	const int32 Lanes[2] = { PLane - 1, PLane + 1 };

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 i = 0; i < 2; ++i)
	{
		const int32 L = Lanes[i];

		// 足場が無ければ、その位置の空中から上げる
		const FVector Loc = Terrain->GetStepLocation(PRow, L)
			+ FVector(0.f, 0.f, 55.f);

		if (AStairSpark* S = World->SpawnActor<AStairSpark>(
			AStairSpark::StaticClass(), Loc, FRotator::ZeroRotator, SP))
		{
			// ★コンボが伸びるほど派手にする。色は粒ごとに virandom で散らす
			const float T = FMath::Clamp(Combo / 60.f, 0.f, 1.f);
			S->BurstRainbow(1.4f + T * 1.1f);
		}
	}
}

void AStairGameMode::NotifyPlayerFell()
{
	if (State != EStairGameState::Playing)
	{
		return;
	}
	EndGame(EStairEndReason::Fell);
}

void AStairGameMode::NotifyMissOnCurrentStep()
{
	AStairCharacter* P = GetPlayerChar();
	if (State != EStairGameState::Playing || !P || !Terrain || !Config)
	{
		return;
	}

	++MissCount;
	++JumpMissCount;   // 拍を外した・壁に弾かれた

	AStairStep* Here = Terrain->GetStep(P->GetCurrentRow(), P->GetCurrentLane());
	if (!Here)
	{
		return;
	}

	// 赤マスは崩落対象外
	if (Config->bRedTilesNeverCollapse && !Here->CanCollapse())
	{
		return;
	}

	if (Here->RegisterMiss(Config->MissLimit))
	{
		// 上限に達した。足場が崩れる
		const int32 R = Here->Row;
		const int32 L = Here->Lane;
		Terrain->RemoveStep(R, L);
		EndGame(EStairEndReason::Collapsed);
	}
	else if (Here->GetMissCount() >= Config->MissLimit - 1)
	{
		// ★次のMISSで崩れる。警告音を鳴らす
		PlaySystemSound(Config->WarningSound);
	}
}

// =====================================================================
// 演出。★背景レイヤーだけを暗くする
// =====================================================================

void AStairGameMode::UpdateSkyByT(float T)
{
	if (!Config)
	{
		return;
	}

	const float A = FMath::Clamp(T, 0.f, 1.f);

	if (SunLight)
	{
		if (UDirectionalLightComponent* C = SunLight->GetComponent())
		{
			C->SetIntensity(FMath::Lerp(Config->SunIntensityStart,
				Config->SunIntensityEnd, A));
			C->SetLightColor(FMath::Lerp(Config->SkyColorStart,
				Config->SkyColorEnd, A * 0.7f));
			// 日が沈んでいく
			const float Pitch = FMath::Lerp(-42.f, -4.f, A);
			SunLight->SetActorRotation(FRotator(Pitch, -35.f, 0.f));
		}
	}

	if (Fog)
	{
		if (UExponentialHeightFogComponent* C = Fog->GetComponent())
		{
			C->SetFogInscatteringColor(FMath::Lerp(Config->FogColorStart,
				Config->FogColorEnd, A));
			C->SetFogDensity(FMath::Lerp(0.02f, 0.06f, A));
		}
	}

	if (SkyLightActor)
	{
		if (USkyLightComponent* C = SkyLightActor->GetLightComponent())
		{
			C->SetIntensity(FMath::Lerp(1.0f, 0.12f, A));
			C->SetLightColor(FMath::Lerp(Config->SkyColorStart,
				Config->SkyColorEnd, A));
		}
	}
}

// =====================================================================
// UI
// =====================================================================

void AStairGameMode::UpdateWidgets()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return;
	}

	// ★テンポ合わせの案内も HUD に出すので、Intro を含める。
	//   ここに入れ忘れると、曲を選んだあと画面に何も出ず、
	//   何をすればよいか分からないまま止まって見える。
	const bool bWantHUD = (State == EStairGameState::Intro
		|| State == EStairGameState::Countdown
		|| State == EStairGameState::Playing
		|| State == EStairGameState::Finished);

	if (bWantHUD && !HUDWidget && HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UUserWidget>(PC, HUDWidgetClass);
		if (HUDWidget) { HUDWidget->AddToViewport(0); }
	}
	else if (!bWantHUD && HUDWidget)
	{
		HUDWidget->RemoveFromParent();
		HUDWidget = nullptr;
	}

	const bool bWantResult = (State == EStairGameState::Result);
	if (bWantResult && !ResultWidget && ResultWidgetClass)
	{
		ResultWidget = CreateWidget<UUserWidget>(PC, ResultWidgetClass);
		if (ResultWidget)
		{
			ResultWidget->AddToViewport(20);
			// ★Result中はUIのみ。ゲーム入力は届かない
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

// =====================================================================
// 遷移
// =====================================================================

void AStairGameMode::GoToTitle()
{
	UGameplayStatics::OpenLevel(this, TEXT("L_Title"));
}

void AStairGameMode::GoToSongSelect()
{
	// ★曲選択はタイトル画面のパネルに移した。
	//   古い L_BGMSelect は4曲固定なので使わない。
	if (UStairGameInstance* GI = Cast<UStairGameInstance>(GetGameInstance()))
	{
		GI->bOpenSongSelectOnTitle = true;
	}
	UGameplayStatics::OpenLevel(this, TEXT("L_Title"));
}

void AStairGameMode::RetryGame()
{
	UGameplayStatics::OpenLevel(this, TEXT("L_Game"));
}
