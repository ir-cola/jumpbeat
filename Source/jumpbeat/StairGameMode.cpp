#include "StairGameMode.h"
#include "StairStep.h"
#include "StairCharacter.h"
#include "StairPlayerController.h"
#include "StairConfig.h"
#include "StairTerrain.h"
#include "StairMusicClock.h"
#include "StairWidgets.h"
#include "StairGameInstance.h"
#include "StairChartFile.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "StairSpark.h"
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
#include "Camera/PlayerCameraManager.h"

AStairGameMode::AStairGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultPawnClass = AStairCharacter::StaticClass();
	PlayerControllerClass = AStairPlayerController::StaticClass();

	HUDWidgetClass = UStairHUDWidget::StaticClass();
	ResultWidgetClass = UStairResultWidget::StaticClass();

	MusicClock = CreateDefaultSubobject<UStairMusicClock>(TEXT("MusicClock"));
	Terrain = CreateDefaultSubobject<UStairTerrain>(TEXT("Terrain"));

	PauseWidgetClass = UStairPauseWidget::StaticClass();
}

void AStairGameMode::BeginPlay()
{
	Super::BeginPlay();

	Score = 0;
	PerfectCount = GreatCount = MissCount = 0;
	JumpMissCount = 0;
	Combo = MaxCombo = 0;
	EndReason = EStairEndReason::None;

	CacheSceneActors();

	// ---- 選ばれた曲を取り出す ----
	int32 SongIndex = 0;
	bEndless = false;
	SongsPlayed = 0;
	RowShiftTotal = 0;

	if (UStairGameInstance* GI = Cast<UStairGameInstance>(GetGameInstance()))
	{
		SongIndex = GI->SelectedSongIndex;
		bEndless = GI->bEndlessMode;
	}

	// ★エンドレスは必ず1曲目から。以降は順番に回す
	if (bEndless) { SongIndex = 0; }
	EndlessSongIndex = SongIndex;

	LoadSong(SongIndex);

	// ---- 地形 ----
	if (Terrain)
	{
		Terrain->Initialize(Config);
		if (!Terrain->StepClass)
		{
			Terrain->StepClass = AStairStep::StaticClass();
		}

		// ★譜面の道を先に計算してから地形を作る。
		//   道の上は必ず足場になり、それ以外はランダムのまま。
		BuildChartPath();

		Terrain->UpdateAround(0, 0, 0.f);
	}

	// ---- プレイヤー ----
	// ★ここで取れないことがある。取れるまで Tick で試し続ける
	bPlayerPlaced = false;
	TryInitPlayer();

	// ★前のレベル（BGM選択やリザルト）で UIOnly にした入力モードを必ず戻す。
	//   これを忘れると、曲選択から入り直したときにキー入力が届かなくなる。
	RestoreGameInput();

	// ★幕（UMG）が出そろうまでの数フレームを、カメラ側でも隠す。
	//   これが無いと、切り替わった直後に素の画面が一瞬映る。
	StartScreenFade(FLinearColor::Black);

	// ---- カウントダウンして曲へ ----
	//   ★テンポ合わせのフェーズは廃止した。
	//     画面のサークルワイプが開ききってから少し待ち、
	//     拍に合わせた 3・2・1 が入り、START と同時に曲が始まる。
	{
		const float Beat = 60.f / FMath::Max(1.f, CurrentSong.BPM)
			* (Config ? Config->BeatsPerJudge : 2.f);

		// 画面が開くのを待つぶん ＋ 3拍ぶん
		StartCurrentSong((Config ? Config->CountdownLeadSeconds : 1.f) + Beat * 3.f);
	}

	LastCountdownNumber = 0;
	SetState(EStairGameState::Countdown);
}

void AStairGameMode::LoadSong(int32 Index)
{
	if (Config && Config->Songs.IsValidIndex(Index))
	{
		CurrentSong = Config->Songs[Index];
	}

	// ★譜面エディタで打ったものがあれば、そちらを使う。
	//   打ってすぐ試せるようにするため。Config の中身は
	//   Tools/import_chart.py で焼き込んだ製品版用の控え。
	if (StairChartFile::Load(Index, CurrentSong.Notes))
	{
		UE_LOG(LogTemp, Log, TEXT("[譜面] 書き出し済みを読み込みました（%d 個）: %s"),
			CurrentSong.Notes.Num(), *StairChartFile::PathFor(Index));
	}

	NextNoteIndex = 0;
}

void AStairGameMode::StartCurrentSong(float Lead)
{
	if (!MusicClock)
	{
		return;
	}

	MusicClock->SetGlobalOffset(Config ? Config->AudioOffsetMs * 0.001f : 0.f);
	MusicClock->SetBeatScale(Config ? Config->BeatsPerJudge : 2.f);
	MusicClock->StartSong(CurrentSong.Sound, CurrentSong.BPM,
		CurrentSong.BeatOffset, Lead);
}

bool AStairGameMode::IsBetweenSongs() const
{
	// ★曲を1つ以上流し終えたあとだけ。
	//   1曲目の「まだ鳴っていない」と区別しないと、
	//   START と同時に押した最初の1歩が弾かれてしまう。
	return bEndless && SongsPlayed > 0
		&& State == EStairGameState::Playing
		&& MusicClock && !MusicClock->HasStarted();
}

void AStairGameMode::AdvanceEndlessSong()
{
	if (!Config || Config->Songs.Num() == 0)
	{
		EndGame(EStairEndReason::SongEnd);
		return;
	}

	++SongsPlayed;

	// ★曲を順番に回す。最後まで行ったら1曲目へ戻る
	EndlessSongIndex = (EndlessSongIndex + 1) % Config->Songs.Num();
	LoadSong(EndlessSongIndex);

	// 地形は完全ランダムのまま。譜面は判定と方向にだけ使う
	StartCurrentSong(Config->EndlessGapSeconds);

	UE_LOG(LogTemp, Log, TEXT("[エンドレス] %d 曲目: %s"),
		SongsPlayed + 1, *CurrentSong.Title);
}

void AStairGameMode::StartScreenFade(const FLinearColor& Color)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->SetManualCameraFade(1.f, Color, false);
		}
	}
	ScreenFadeHold = Config ? FMath::Max(0.05f, Config->IrisHoldSeconds) : 0.1f;
}

void AStairGameMode::TickScreenFade(float DeltaSeconds)
{
	if (ScreenFadeHold <= 0.f)
	{
		return;
	}

	ScreenFadeHold -= DeltaSeconds;
	if (ScreenFadeHold > 0.f)
	{
		return;
	}

	// 幕（UMG）が出そろったので、カメラ側の暗転は解く
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StopCameraFade();
		}
	}
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
	return MusicClock ? MusicClock->GetT() : 0.f;
}

float AStairGameMode::GetRemainingTime() const
{
	if (!MusicClock)
	{
		return 0.f;
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

float AStairGameMode::GetSongTimeNow() const
{
	return MusicClock ? MusicClock->GetSongTime() : 0.f;
}

float AStairGameMode::GetSignedJudgeOffsetAt(float AtSongTime, int32 NoteIndex) const
{
	if (!MusicClock)
	{
		return 0.f;
	}

	// -1 なら「いま待っている音符」
	const int32 Idx = (NoteIndex >= 0) ? NoteIndex : NextNoteIndex;

	// ★指定された音符との差。
	//   マイナス＝音符より早い（FAST）、プラス＝遅い（SLOW）。
	if (HasChart() && State == EStairGameState::Playing
		&& CurrentSong.Notes.IsValidIndex(Idx))
	{
		return AtSongTime - NoteTime(Idx);
	}
	return MusicClock->GetOffsetFromNearestBeat();
}

float AStairGameMode::GetSignedJudgeOffset() const
{
	return GetSignedJudgeOffsetAt(GetSongTimeNow());
}

EStairJudge AStairGameMode::JudgeAt(float AtSongTime, int32 NoteIndex) const
{
	if (!MusicClock || !Config)
	{
		return EStairJudge::Miss;
	}

	// ★指定された音符が既に流れ落ちていたら、その入力は無効。
	//   次の音符を横取りさせない。
	if (NoteIndex >= 0 && NoteIndex < NextNoteIndex)
	{
		return EStairJudge::Miss;
	}

	// 一定間隔の拍ではなく、置かれた音符どおりに跳ぶゲームにするため。
	// 譜面が無い曲は従来どおり全拍で跳べる。
	const float OffsetMs =
		FMath::Abs(GetSignedJudgeOffsetAt(AtSongTime, NoteIndex)) * 1000.f;

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

EStairJudge AStairGameMode::JudgeNow() const
{
	return JudgeAt(GetSongTimeNow());
}

bool AStairGameMode::DoesDirectionMatch(EStairDir Dir, int32 NoteIndex) const
{
	// ★エンドレスは地形が完全ランダムなので、方向は問わない。
	//   譜面どおりの向きに跳ぶと穴や壁に突っ込むだけで、
	//   その通りに叩くと進めなくなってしまう。
	//   叩くタイミングだけを見る。
	if (bEndless)
	{
		return true;
	}

	const int32 Idx = (NoteIndex >= 0) ? NoteIndex : NextNoteIndex;

	// 譜面が無ければ好きな方向へ跳べる
	if (!HasChart() || !CurrentSong.Notes.IsValidIndex(Idx))
	{
		return true;
	}

	switch (CurrentSong.Notes[Idx].Type)
	{
	case EStairNote::Left:  return Dir == EStairDir::Left;
	case EStairNote::Right: return Dir == EStairDir::Right;
	default:                return Dir == EStairDir::Forward;   // 正面と赤
	}
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

	// 切り替え直後の暗転を、幕が出そろったら解く
	TickScreenFade(DeltaSeconds);

	// Pawn がまだ取れていなければ取れるまで試す
	TryInitPlayer();

#if STAIR_DEV_TOOLS
	// ★開発用の隠し操作。製品版では丸ごと無効。
	//   遊んでいる人が O を誤って押すと地形が全部床に変わってしまう。
	UpdateTimingTuner();
	UpdateCharting();
	TunerShowTime = FMath::Max(0.f, TunerShowTime - DeltaSeconds);
#endif

	// t は1箇所でだけ求め、全システムに配る
	const float T = GetT();
	UpdateSkyByT(T);

	switch (State)
	{
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

		// 残像とゲージの退場
		UpdateGaugeState(DeltaSeconds);

		// ★叩かれないまま通り過ぎた音符を落とす。
		//   ここが「譜面どおりに叩けなかった」唯一の確定点なので、
		//   コンボを切るのも地形を詰めるのもここで行う。
		if (HasChart() && Config)
		{
			const float Late = Config->GreatWindowMs * 0.001f;
			while (CurrentSong.Notes.IsValidIndex(NextNoteIndex)
				&& GetTimeToNextNote() < -Late)
			{
				OnNoteMissed(NextNoteIndex);
				++NextNoteIndex;
			}
		}

		if (MusicClock && MusicClock->IsSongFinished())
		{
			// ★エンドレスは曲が終わっても終わらない。次の曲へ移るだけ
			if (bEndless)
			{
				AdvanceEndlessSong();
			}
			else
			{
				EndGame(EStairEndReason::SongEnd);
			}
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

	// ★譜面の打ち込み中は終わらせない。
	//   曲を最後まで流し続けられないと、後半の譜面が打てない。
	if (bCharting)
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
	// ★MISS で行を詰めた回数を足し戻す。
	//   詰めると足場の Row が振り直されるので、そのままでは
	//   のぼった段数が目減りしてしまう。
	Score = FMath::Max(Score, Step->Row + RowShiftTotal);
}

void AStairGameMode::NotifyJudge(EStairJudge Judge, float SignedOffset, int32 NoteIndex)
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

	// ★早かったか遅かったかを覚えておく。GREAT の FAST／SLOW 表示に使う
	LastJudgeOffset = SignedOffset;

	// ★叩いた音符はその場で消費する。
	//   残したままだと、判定窓のあいだに続けて押したとき
	//   同じ音符が二度取れてしまう。
	if (HasChart() && Judge != EStairJudge::Miss && Config
		&& FMath::Abs(SignedOffset) <= Config->GreatWindowMs * 0.001f)
	{
		// 叩いた音符の次へ進める。持ち越した入力でも取りこぼさない
		const int32 Idx = (NoteIndex >= 0) ? NoteIndex : NextNoteIndex;
		NextNoteIndex = FMath::Max(NextNoteIndex, Idx + 1);
	}

	UpdateCombo(Judge);

	// ★押した位置を残像として覚えておく。
	//   走り気味か遅れ気味かが、ゲージ上で目に見えるようになる。
	if (MusicClock)
	{
		FStairGhost G;
		G.Offset = LastJudgeOffset;
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

	// ★打ち込み中はゲームを止める。
	//   跳ばせたままだと、聴きながら置く作業と操作が両立せず、
	//   落ちた瞬間に曲が止まって最後まで打てない。
	if (AStairCharacter* P = GetPlayerChar())
	{
		P->SetControlEnabled(!bCharting);
	}

	if (bCharting)
	{
		// いま選んでいる曲の譜面を読み込んで、続きから打てるようにする
		ChartNotes = CurrentSong.Notes;

		// ★打ち込み中は全部床にする。穴や壁があると置きたい位置まで進めない
		if (Terrain)
		{
			Terrain->bAllFloor = true;
			Terrain->ClearChartPath();
		}

		UE_LOG(LogTemp, Warning,
			TEXT("[譜面] 編集開始。既存 %d 個。SPACE=正面 A=左 D=右 W=赤 / BackSpace 取消 / P 書き出し / O 終了"),
			ChartNotes.Num());
	}
	else
	{
		if (Terrain)
		{
			Terrain->bAllFloor = false;
		}
		UE_LOG(LogTemp, Warning, TEXT("[譜面] 編集終了。ゲームを再開します"));
	}
}

float AStairGameMode::NoteTime(int32 Index) const
{
	if (!MusicClock || !Config || !CurrentSong.Notes.IsValidIndex(Index))
	{
		return BIG_NUMBER;
	}
	const int32 Sub = FMath::Max(1, Config->ChartSubdivision);
	const float Beat = MusicClock->GetBeatDuration();
	return (float(CurrentSong.Notes[Index].Slot) / Sub) * Beat + CurrentSong.BeatOffset;
}

float AStairGameMode::GetTimeToNextNote() const
{
	if (!MusicClock || !HasChart())
	{
		return BIG_NUMBER;
	}
	if (!CurrentSong.Notes.IsValidIndex(NextNoteIndex))
	{
		return BIG_NUMBER;
	}
	return NoteTime(NextNoteIndex) - MusicClock->GetSongTime();
}

float AStairGameMode::GetNoteTimeFromNow(int32 Index) const
{
	if (!MusicClock || !CurrentSong.Notes.IsValidIndex(Index))
	{
		return BIG_NUMBER;
	}
	return NoteTime(Index) - MusicClock->GetSongTime();
}

EStairNote AStairGameMode::GetNoteType(int32 Index) const
{
	return CurrentSong.Notes.IsValidIndex(Index)
		? CurrentSong.Notes[Index].Type : EStairNote::Forward;
}

int32 AStairGameMode::GetStepsPerNote() const
{
	// ★譜面どおりの道を敷いているときの段数。
	//   エンドレスは道を敷かないので、この値は使わない
	//   （判定ごとに PerfectSteps / GreatSteps で決まる）。
	return Config ? FMath::Max(1, Config->ChartStepsPerNote) : 1;
}

bool AStairGameMode::TryStartFromCountdown()
{
	if (State != EStairGameState::Countdown || !MusicClock || !Config)
	{
		return false;
	}

	// ★START のちょうど手前で押されたら、そこから遊びを始める。
	//   曲が鳴り出すのを Tick で待っていたので、
	//   START と同時に押した1歩めが空押し扱いで捨てられていた。
	const float Left = MusicClock->GetCountdownRemaining();
	if (Left > Config->GreatWindowMs * 0.001f)
	{
		return false;   // まだ早い。空押しとして拍を確かめるだけ
	}

	SetState(EStairGameState::Playing);
	return true;
}

float AStairGameMode::GetNextNoteWindow() const
{
	if (!MusicClock || !Config || !HasChart())
	{
		return BIG_NUMBER;
	}

	// ★NextNoteIndex は常に「次に押すべき音符」を指している。
	//
	//   叩けた音符は NotifyJudge が先に消費して1つ進めているので、
	//   ここでさらに1つ先を見てはいけない。
	//   見てしまうと滞空が長く見積もられ、
	//   そのあいだに次の音符が流れ切ってしまう。
	if (!CurrentSong.Notes.IsValidIndex(NextNoteIndex))
	{
		return BIG_NUMBER;    // これが最後。急ぐ必要はない
	}

	// ★「いまから」何秒あるか。遅れて押したぶんは短くなる
	return FMath::Max(0.f, GetNoteTimeFromNow(NextNoteIndex));
}

EStairNote AStairGameMode::GetNextNoteType() const
{
	if (CurrentSong.Notes.IsValidIndex(NextNoteIndex))
	{
		return CurrentSong.Notes[NextNoteIndex].Type;
	}
	return EStairNote::Forward;
}

void AStairGameMode::BuildChartPath()
{
	if (!Terrain || !Config)
	{
		return;
	}

	// ★エンドレスは地形を完全ランダムにする。譜面は判定と方向にだけ使う
	if (!HasChartRoad())
	{
		Terrain->ClearChartPath();
		Terrain->ClearLaneBounds();
		return;
	}

	// ★譜面のとおりに跳んだときに通る道を、先に全部たどっておく。
	//   譜面どおりに PERFECT を出せば、必ずここを通って進める。
	//
	//   進む段数は音符の種類だけで決まる。判定（PERFECT/GREAT）では
	//   変えない。変えてしまうと着地点が二通りになり、
	//   「譜面どおりに進む道」が定まらなくなるため。
	TMap<int64, EStairTile> Path;    // 必ず足場にするマス
	TSet<int64> Clear;               // 壁を置いてはいけないマス
	TSet<int32> RedRows;             // 横一列すべてを赤にする行
	TSet<int32> HoleRows;            // 赤から跳び越す谷。丸ごと穴にする行

	int32 Row = 0;
	int32 Lane = 0;
	int32 MinLane = 0;
	int32 MaxLane = 0;

	// 出発点
	Path.Add(UStairTerrain::MakeKey(Row, Lane), EStairTile::Normal);

	const int32 Steps = GetStepsPerNote();

	for (const FStairChartNote& N : CurrentSong.Notes)
	{
		if (N.Type == EStairNote::Red)
		{
			// ★赤は「いま立っている行が赤で、そこから5段先へ跳ぶ」という意味。
			//   その行は横一列すべてを赤にする。
			//   長距離ジャンプの踏切であることが遠目にも分かるようにするため。
			RedRows.Add(Row);

			// ★踏切から着地点の手前までは丸ごと穴。
			//   跳び越えるための谷を見せて、5段跳びの理由を分からせる。
			for (int32 R = Row + 1; R < Row + Config->RedSteps; ++R)
			{
				HoleRows.Add(R);
			}

			Row += Config->RedSteps;
		}
		else
		{
			const int32 NextLane = Lane
				+ ((N.Type == EStairNote::Left) ? -1
				: (N.Type == EStairNote::Right) ? 1 : 0);

			// ★まっすぐ跳ぶときだけ、途中の段に壁があると引っかかる。
			//   斜めは桂馬の動きで回り込むので、途中の段は気にしなくてよい。
			if (NextLane == Lane)
			{
				for (int32 R = Row + 1; R < Row + Steps; ++R)
				{
					Clear.Add(UStairTerrain::MakeKey(R, Lane));
				}
			}

			Row += Steps;
			Lane = NextLane;
		}

		// 着地点
		const int64 K = UStairTerrain::MakeKey(Row, Lane);
		if (!Path.Contains(K))
		{
			Path.Add(K, EStairTile::Normal);
		}

		MinLane = FMath::Min(MinLane, Lane);
		MaxLane = FMath::Max(MaxLane, Lane);
	}

	Terrain->SetChartPath(Path, Clear, RedRows, HoleRows);

	// ★横は譜面が使う幅ぶんだけ。無限に広げても見えないうえに重くなる
	const int32 Margin = FMath::Max(0, Config->ChartLaneMargin);
	Terrain->SetLaneBounds(MinLane - Margin, MaxLane + Margin);

	UE_LOG(LogTemp, Warning,
		TEXT("[譜面] 音符 %d 個 → 足場 %d マス / 赤 %d 行 / 谷 %d 行 / レーン %d〜%d"),
		CurrentSong.Notes.Num(), Path.Num(), RedRows.Num(), HoleRows.Num(),
		MinLane - Margin, MaxLane + Margin);
}

void AStairGameMode::ChartPlace()
{
	ChartPlaceTyped(EStairNote::Forward);
}

void AStairGameMode::ChartPlaceTyped(EStairNote Type)
{
	if (!bCharting || !MusicClock)
	{
		return;
	}

	// ★押した時刻を最寄りの位置に丸める。
	//   1拍を ChartSubdivision で割った細かさなので、裏拍にも置ける。
	const int32 Sub = Config ? FMath::Max(1, Config->ChartSubdivision) : 4;
	const int32 Slot = FMath::RoundToInt(MusicClock->GetBeatPosition() * Sub);

	// 同じ位置には1つだけ。押し直したら種類を差し替える
	for (FStairChartNote& N : ChartNotes)
	{
		if (N.Slot == Slot)
		{
			N.Type = Type;
			return;
		}
	}

	FStairChartNote N;
	N.Slot = Slot;
	N.Type = Type;
	ChartNotes.Add(N);
	ChartNotes.Sort();

	PlaySystemSound(Config ? Config->CountdownSound : nullptr);

	static const TCHAR* Names[] = { TEXT("正面"), TEXT("左"), TEXT("右"), TEXT("赤") };
	UE_LOG(LogTemp, Warning, TEXT("[譜面] %d 拍目 %d/%d に %s（計 %d 個）"),
		Slot / Sub, Slot % Sub, Sub, Names[(int32)Type], ChartNotes.Num());
}

void AStairGameMode::ChartUndo()
{
	if (!bCharting || ChartNotes.Num() == 0)
	{
		return;
	}

	// いまの時刻にいちばん近いものを消す
	const int32 Sub = Config ? FMath::Max(1, Config->ChartSubdivision) : 4;
	int32 Best = ChartNotes.Num() - 1;

	if (MusicClock)
	{
		const float Now = MusicClock->GetBeatPosition() * Sub;
		float BestDiff = BIG_NUMBER;
		for (int32 i = 0; i < ChartNotes.Num(); ++i)
		{
			const float D = FMath::Abs(ChartNotes[i].Slot - Now);
			if (D < BestDiff) { BestDiff = D; Best = i; }
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[譜面] %d 拍目を取消（残り %d 個）"),
		ChartNotes[Best].Slot / Sub, ChartNotes.Num() - 1);
	ChartNotes.RemoveAt(Best);
}

void AStairGameMode::ChartExport()
{
	if (ChartNotes.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[譜面] 空なので書き出しません"));
		return;
	}

	// 「位置:種類」の並びにする。種類は F/L/R/X
	static const TCHAR* Codes[] = { TEXT("F"), TEXT("L"), TEXT("R"), TEXT("X") };

	FString Line;
	for (int32 i = 0; i < ChartNotes.Num(); ++i)
	{
		Line += FString::Printf(TEXT("%d:%s"),
			ChartNotes[i].Slot, Codes[(int32)ChartNotes[i].Type]);
		if (i < ChartNotes.Num() - 1) { Line += TEXT(","); }
	}

	const int32 Sub = Config ? FMath::Max(1, Config->ChartSubdivision) : 4;

	const FString Text = FString::Printf(
		TEXT("曲: %s\nBPM: %.2f\n分割: 1拍を %d 分割\n個数: %d\n")
		TEXT("種類: F=正面 L=左 R=右 X=赤マス\nNotes:\n%s\n"),
		*CurrentSong.Title, CurrentSong.BPM, Sub, ChartNotes.Num(), *Line);

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

	const int32 Sub = Config ? FMath::Max(1, Config->ChartSubdivision) : 4;
	const int32 Slot = MusicClock
		? FMath::RoundToInt(MusicClock->GetBeatPosition() * Sub) : 0;

	return FString::Printf(
		TEXT("譜面編集中　%d 拍目 %d/%d　置いた数 %d\n")
		TEXT("SPACE=正面　A=左　D=右　W=赤マス\n")
		TEXT("BackSpace=取消　P=書き出し　O=終了"),
		Slot / Sub, Slot % Sub, Sub, ChartNotes.Num());
}

void AStairGameMode::UpdateCharting()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	// ★編集モードに入れるのはカウントダウン中だけ。
	//   プレイ中に切り替えられると、遊んでいる最中に地形が
	//   全床へ変わってしまい、何が起きたか分からなくなる。
	//   抜けるのはいつでもできるようにしておく。
	if (PC->WasInputKeyJustPressed(EKeys::O))
	{
		if (bCharting || State == EStairGameState::Countdown)
		{
			ChartToggle();
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[譜面] 編集に入れるのはカウントダウン中だけです"));
		}
	}

	if (!bCharting)
	{
		return;
	}

	// ★音符の種類はキーで決める。
	//   実際に遊ぶときの操作と同じキーにしておくと、
	//   打ち込みながら「どう跳ぶか」がそのまま身につく。
	if (PC->WasInputKeyJustPressed(EKeys::SpaceBar)) { ChartPlaceTyped(EStairNote::Forward); }
	if (PC->WasInputKeyJustPressed(EKeys::A))        { ChartPlaceTyped(EStairNote::Left); }
	if (PC->WasInputKeyJustPressed(EKeys::D))        { ChartPlaceTyped(EStairNote::Right); }
	if (PC->WasInputKeyJustPressed(EKeys::W))        { ChartPlaceTyped(EStairNote::Red); }

	if (PC->WasInputKeyJustPressed(EKeys::BackSpace)) { ChartUndo(); }
	if (PC->WasInputKeyJustPressed(EKeys::P))         { ChartExport(); }
}

void AStairGameMode::TogglePause()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	if (State == EStairGameState::Paused)
	{
		// ---- 再開 ----
		State = StateBeforePause;

		if (PauseWidget)
		{
			PauseWidget->RemoveFromParent();
			PauseWidget = nullptr;
		}

		UGameplayStatics::SetGamePaused(this, false);
		if (MusicClock) { MusicClock->SetPaused(false); }
		RestoreGameInput();
		return;
	}

	// ポーズできるのは遊んでいる最中だけ
	if (State != EStairGameState::Playing && State != EStairGameState::Countdown)
	{
		return;
	}

	// ---- 止める ----
	StateBeforePause = State;
	State = EStairGameState::Paused;

	if (MusicClock) { MusicClock->SetPaused(true); }

	if (!PauseWidgetClass)
	{
		PauseWidgetClass = UStairPauseWidget::StaticClass();
	}
	PauseWidget = CreateWidget<UUserWidget>(PC, PauseWidgetClass);
	if (PauseWidget)
	{
		PauseWidget->AddToViewport(50);
		UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(
			PC, PauseWidget, EMouseLockMode::DoNotLock);
		PC->bShowMouseCursor = true;
	}

	// ★ゲームを止める。Tick が来なくなるので曲の位置も保たれる
	UGameplayStatics::SetGamePaused(this, true);
}

void AStairGameMode::NotifyPracticeTap()
{
	if (!MusicClock || !Config)
	{
		return;
	}

	// 進みはしない。押した位置だけ残像に残して、拍を確かめられるようにする
	FStairGhost G;
	G.Offset = GetSignedJudgeOffset();
	G.Age = 0.f;
	G.Judge = JudgeNow();
	Ghosts.Insert(G, 0);

	const int32 Keep = FMath::Max(1, Config->GhostCount);
	if (Ghosts.Num() > Keep)
	{
		Ghosts.SetNum(Keep);
	}

	if (Config->JumpSound)
	{
		float Pitch = Config->JumpPitchGreat;
		if (G.Judge == EStairJudge::Perfect) { Pitch = Config->JumpPitchPerfect; }
		else if (G.Judge == EStairJudge::Miss) { Pitch = Config->JumpPitchMiss; }

		UGameplayStatics::PlaySound2D(this, Config->JumpSound,
			Config->JumpSoundVolume * 0.6f, Pitch);
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

	// ★ゲージは最後まで出しっぱなしにする。
	//   タイミングを体で覚えるゲームではなく、譜面を読む音ゲーになったので、
	//   途中で消すと何を見て跳べばよいのか分からなくなる。
}

void AStairGameMode::CollapseRows(int32 Count, bool bClearRedUnderPlayer)
{
	// ★譜面どおりの道を敷いているときだけ。
	//   エンドレスは完全ランダムなので、合わせ直す相手がいない。
	if (!HasChartRoad() || !Terrain || !Config || Count <= 0)
	{
		return;
	}

	AStairCharacter* P = GetPlayerChar();
	if (!P)
	{
		return;
	}

	// ★跳んでいる最中なら着地予定の行を基準にする。
	//
	//   音符が詰まっていると、跳んでいるあいだに次の音符が流れ切ることがある。
	//   そのとき CurrentRow は跳ぶ前の値のままなので、
	//   Row+1 ＝ これから降りる足場を壊してしまい、
	//   着地に失敗したり以降の位置がずれたりしていた。
	const int32 Row = P->GetEffectiveRow();

	// ★赤の音符を落としたときは、足元が赤のまま残る。
	//   放っておくと次のジャンプも5段になって永久にずれるので解除する。
	if (bClearRedUnderPlayer)
	{
		Terrain->ClearRedRow(Row);
	}

	const int32 Front = Row + 1;

	for (int32 i = 0; i < Count; ++i)
	{
		Terrain->CollapseRow(Front);
		++RowShiftTotal;
	}
}

void AStairGameMode::OnNoteMissed(int32 Index)
{
	if (State != EStairGameState::Playing)
	{
		return;
	}

	++MissCount;
	++JumpMissCount;

	// ★譜面どおりに叩けなかったのでコンボは切れる。
	//   立ち止まっていても切れるよう、判定ではなくここで切る。
	Combo = 0;
	LastComboBeat = -9999;

	// 見逃した音符ぶんだけ地形を詰めて、この先の譜面と合わせ直す
	const bool bRed = (GetNoteType(Index) == EStairNote::Red);

	// ★次の音符も赤なら、足元の赤は残す。
	//
	//   赤が続く区間では、次の赤マスへ跳ぶための踏切がそのまま要る。
	//   ここで赤を解除すると1段しか跳べなくなり、
	//   目の前の谷に落ちるか、跳べずに詰んでしまう。
	//   赤のまま残せば、次の赤の音符でそのまま次の赤マスまで跳んでいける。
	const bool bNextRed = CurrentSong.Notes.IsValidIndex(Index + 1)
		&& (GetNoteType(Index + 1) == EStairNote::Red);

	// ★横のずれも詰める。
	//
	//   左右の音符を落とすと、譜面の道は横へ1つ進むのにプレイヤーは
	//   その場に残る。行を詰めても縦しか直らないので、
	//   このままだと以降ずっと1レーンずれた道を歩くことになり、
	//   穴や壁に突っ込んで進めなくなる。
	if (AStairCharacter* P = GetPlayerChar())
	{
		int32 NoteLane = 0;
		switch (GetNoteType(Index))
		{
		case EStairNote::Left:  NoteLane = -1; break;
		case EStairNote::Right: NoteLane = 1;  break;
		default: break;
		}

		if (NoteLane != 0 && HasChartRoad() && Terrain)
		{
			// 譜面が寄ったぶんだけ、前方を逆へ寄せて帳尻を合わせる
			Terrain->ShiftLanesAbove(P->GetEffectiveRow(), -NoteLane);
		}
	}

	CollapseRows(bRed ? FMath::Max(1, Config ? Config->RedSteps : 5)
	                  : GetStepsPerNote(),
	             bRed && !bNextRed);

	// 見逃しても足場は傷む。立ち止まり続ければいずれ崩れる
	DamageCurrentStep();
}

void AStairGameMode::NotifyWallBounce()
{
	// ★判定は壁に当たる前に通知済みなので、コンボはここで切る。
	//   叩けてはいるが1段も進んでいないので、繋がったことにはしない。
	Combo = 0;
	LastComboBeat = -9999;

	// ★壁に弾かれた場合、音符は「叩けた」ので既に消費されている。
	//   進めていないぶんだけ地形を詰めないと、この先がずれる。
	NotifyMissOnCurrentStep();
	CollapseRows(GetStepsPerNote(), false);
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

	// ★譜面があるときは「音符を1つも落とさずに続けた数」を数える。
	//   音符は等間隔とは限らないので、拍で数えると
	//   譜面どおりに叩けていてもコンボが切れてしまう。
	if (HasChart())
	{
		++Combo;
		MaxCombo = FMath::Max(MaxCombo, Combo);

		const int32 SparkEvery = Config ? FMath::Max(1, Config->ComboSparkEvery) : 10;
		if ((Combo % SparkEvery) == 0)
		{
			SpawnComboSparks();
		}
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

	// ★4か所から打ち上げる。
	//   すぐ左右の2つと、2つ横・3つ奥の2つ。
	//   手前だけだと視界が埋まり、奥だけだと自分の手柄に見えない。
	struct FSpot { int32 Row; int32 Lane; };
	const FSpot Spots[4] =
	{
		{ PRow,     PLane - 1 },
		{ PRow,     PLane + 1 },
		{ PRow + 3, PLane - 2 },
		{ PRow + 3, PLane + 2 }
	};

	// ★足場から直接ではなく、少し浮かせて空中で弾けさせる
	const float Height = Config->SparkHeight;

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// コンボが伸びるほど派手にする
	const float T = FMath::Clamp(Combo / 60.f, 0.f, 1.f);

	for (const FSpot& Spot : Spots)
	{
		// 足場が無くても、その位置の空中から上げる
		const FVector Loc = Terrain->GetStepLocation(Spot.Row, Spot.Lane)
			+ FVector(0.f, 0.f, Height);

		if (AStairSpark* S = World->SpawnActor<AStairSpark>(
			AStairSpark::StaticClass(), Loc, FRotator::ZeroRotator, SP))
		{
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

	// ★ここでは地形を詰めない。
	//   拍と関係ない空押しでも呼ばれるので、詰めると譜面のほうがずれる。
	//   詰めるのは「音符が1つ消えたのに進めなかった」ときだけ。
	DamageCurrentStep();
}

void AStairGameMode::NotifyMissJump()
{
	// ★譜面があるときは何もしない。
	//   同じ1回のミスを「押した瞬間」と「音符が流れ切った瞬間」の
	//   二度数えてしまい、MISS が2回、足場のヒビも2つ入っていた。
	//   数えるのは音符側（OnNoteMissed）だけにする。
	if (HasChart())
	{
		return;
	}
	NotifyMissOnCurrentStep();
}

void AStairGameMode::DamageCurrentStep()
{
	AStairCharacter* P = GetPlayerChar();
	if (State != EStairGameState::Playing || !P || !Terrain || !Config)
	{
		return;
	}

	// 跳んでいる最中なら、これから降りる足場を傷める
	AStairStep* Here = Terrain->GetStep(P->GetEffectiveRow(), P->GetEffectiveLane());
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
		// ★ADirectionalLight::GetComponent() はエディタ専用で、
		//   パッケージ（Shipping）には存在しない。
		//   どの構成でも使える ALight::GetLightComponent() から取る。
		if (UDirectionalLightComponent* C =
			Cast<UDirectionalLightComponent>(SunLight->GetLightComponent()))
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

	const bool bWantHUD = (State == EStairGameState::Countdown
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
