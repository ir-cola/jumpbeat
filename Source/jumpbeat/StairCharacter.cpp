#include "StairCharacter.h"
#include "StairStep.h"
#include "StairGameMode.h"
#include "StairConfig.h"
#include "StairTerrain.h"
#include "StairMusicClock.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

AStairCharacter::AStairCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = 0.f;
		Move->AirControl = 0.f;
		Move->BrakingDecelerationWalking = 100000.f;
		Move->bOrientRotationToMovement = false;
		Move->GravityScale = 2.2f;
	}

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 720.f;
	SpringArm->SetRelativeRotation(FRotator(-34.f, 0.f, 0.f));
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bInheritRoll = false;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 7.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
}

void AStairCharacter::BeginPlay()
{
	Super::BeginPlay();
	SetActorRotation(FRotator::ZeroRotator);

	if (AStairGameMode* GM = GetStairGameMode())
	{
		if (const UStairConfig* C = GM->GetConfig())
		{
			TargetArmLength = C->CameraArmNormal;
			if (SpringArm)
			{
				SpringArm->TargetArmLength = TargetArmLength;
				SpringArm->SetRelativeRotation(FRotator(C->CameraPitch, 0.f, 0.f));
			}

			// ★重力でジャンプの速さを決める。弧の形は変わらない
			if (UCharacterMovementComponent* Move = GetCharacterMovement())
			{
				Move->GravityScale = C->JumpGravityScale;
			}
		}
	}
}

AStairGameMode* AStairCharacter::GetStairGameMode() const
{
	return Cast<AStairGameMode>(UGameplayStatics::GetGameMode(this));
}

void AStairCharacter::SetControlEnabled(bool bEnabled)
{
	bControlEnabled = bEnabled;
}

void AStairCharacter::SetCameraWide(bool bWide)
{
	if (AStairGameMode* GM = GetStairGameMode())
	{
		if (const UStairConfig* C = GM->GetConfig())
		{
			TargetArmLength = bWide ? C->CameraArmWide : C->CameraArmNormal;
			return;
		}
	}
	TargetArmLength = bWide ? 1250.f : 720.f;
}

void AStairCharacter::PlaceOnStep(int32 Row, int32 Lane, const FVector& StepLocation)
{
	CurrentRow = Row;
	CurrentLane = Lane;
	TargetRow = Row;
	TargetLane = Lane;
	bAirborne = false;
	LaunchBaseZ = StepLocation.Z;

	// ★段の厚みとカプセルから正しい高さを求める。ここを間違えるとめり込む
	SetActorLocation(StepLocation + FVector(0.f, 0.f, GetStandZOffset()));
	SetActorRotation(FRotator::ZeroRotator);

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->SetMovementMode(MOVE_Walking);
	}
}

void AStairCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TimeSinceJudge += DeltaSeconds;

	// カメラ距離を滑らかに寄せる
	if (SpringArm)
	{
		SpringArm->TargetArmLength = FMath::FInterpTo(
			SpringArm->TargetArmLength, TargetArmLength, DeltaSeconds, CameraZoomSpeed);
	}

	// ★跳んでいる最中だけ位置を直接進める。
	//   落下はここでは扱わない。物理に任せる。
	if (bScriptedJump)
	{
		TickScriptedJump(DeltaSeconds);
	}

	// 落下＝即死
	if (bAirborne && GetActorLocation().Z < LaunchBaseZ - 600.f)
	{
		bAirborne = false;
		if (AStairGameMode* GM = GetStairGameMode())
		{
			GM->NotifyPlayerFell();
		}
	}
}

void AStairCharacter::PlayJumpSound(EStairJudge Judge)
{
	AStairGameMode* GM = GetStairGameMode();
	if (!GM)
	{
		return;
	}
	const UStairConfig* C = GM->GetConfig();
	if (!C || !C->JumpSound)
	{
		return;
	}

	// 判定に応じてピッチを変える。耳でも判定が分かるようにする
	float Pitch = C->JumpPitchGreat;
	switch (Judge)
	{
	case EStairJudge::Perfect: Pitch = C->JumpPitchPerfect; break;
	case EStairJudge::Miss:    Pitch = C->JumpPitchMiss;    break;
	default: break;
	}

	UGameplayStatics::PlaySound2D(this, C->JumpSound, C->JumpSoundVolume, Pitch);
}

float AStairCharacter::GetStandZOffset() const
{
	// 段の厚みの半分（Cube は 100 ユニット）
	float HalfStep = 22.5f;
	if (const AStairGameMode* GM = GetStairGameMode())
	{
		if (const UStairConfig* C = GM->GetConfig())
		{
			HalfStep = C->StepScale.Z * 100.f * 0.5f;
		}
	}

	// カプセルの半分
	float HalfCapsule = 88.f;
	if (const UCapsuleComponent* Cap = GetCapsuleComponent())
	{
		HalfCapsule = Cap->GetScaledCapsuleHalfHeight();
	}

	// 少し浮かせて確実に段の上に乗せる
	return HalfStep + HalfCapsule + 2.f;
}

FVector AStairCharacter::GetStandLocation(int32 Row, int32 Lane) const
{
	const AStairGameMode* GM = GetStairGameMode();
	if (!GM || !GM->GetTerrain())
	{
		return GetActorLocation();
	}
	return GM->GetTerrain()->GetStepLocation(Row, Lane)
		+ FVector(0.f, 0.f, GetStandZOffset());
}

void AStairCharacter::PlayJumpAnim()
{
	USkeletalMeshComponent* M = GetMesh();
	if (!M)
	{
		return;
	}

	// 元の AnimBP を覚えておく
	if (!DefaultAnimClass)
	{
		DefaultAnimClass = M->GetAnimClass();
	}

	if (!JumpAnim)
	{
		JumpAnim = LoadObject<UAnimSequence>(nullptr,
			TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Jump.MM_Jump"));
	}
	if (!JumpAnim)
	{
		return;
	}

	M->PlayAnimation(JumpAnim, false);

	// 滞空時間ちょうどで1周するよう速さを合わせる
	const float Len = JumpAnim->GetPlayLength();
	if (Len > 0.f)
	{
		M->SetPlayRate(FMath::Clamp(Len / FMath::Max(0.05f, JumpDuration), 0.25f, 4.f));
	}
}

void AStairCharacter::RestoreAnimBlueprint()
{
	USkeletalMeshComponent* M = GetMesh();
	if (!M || !DefaultAnimClass)
	{
		return;
	}

	// AnimBP に戻す。以降は速度と接地状態で自動的に動く
	M->SetPlayRate(1.f);
	M->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	M->SetAnimInstanceClass(DefaultAnimClass);
}

void AStairCharacter::StartScriptedJump(const FVector& Target, float FlightTime,
	float ApexClearance)
{
	JumpFrom = GetActorLocation();
	JumpTo = Target;
	JumpElapsed = 0.f;
	JumpDuration = FMath::Max(0.12f, FlightTime);

	// 頂点は「高いほう＋余裕」。途中の段や壁を越えられる高さにする
	const float Dz = JumpTo.Z - JumpFrom.Z;
	JumpArcHeight = FMath::Max(Dz, 0.f) + FMath::Max(20.f, ApexClearance);

	LaunchBaseZ = JumpFrom.Z;
	bAirborne = true;
	bScriptedJump = true;

	// ★跳んでいるあいだは物理も当たり判定も切る。
	//   壁に引っかかって軌道が崩れるのを根本から防ぐ。
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->SetMovementMode(MOVE_None);
	}
	SetActorEnableCollision(false);

	// ★AnimBP は速度を見ているので、手で動かすと待機モーションになる。
	//   跳躍中だけジャンプのアニメを直接再生する。
	PlayJumpAnim();
}

void AStairCharacter::TickScriptedJump(float DeltaSeconds)
{
	JumpElapsed += DeltaSeconds;

	const float T = FMath::Clamp(JumpElapsed / JumpDuration, 0.f, 1.f);

	// 水平は等速、垂直は放物線。頂点の高さを保証する
	FVector P = FMath::Lerp(JumpFrom, JumpTo, T);
	P.Z += FMath::Sin(PI * T) * JumpArcHeight;

	SetActorLocation(P, false, nullptr, ETeleportType::TeleportPhysics);

	if (T >= 1.f)
	{
		FinishScriptedJump();
	}
}

void AStairCharacter::FinishScriptedJump()
{
	bScriptedJump = false;

	// ★狙った位置ぴったりに置く。ここがずれると次へ進めなくなる
	SetActorLocation(JumpTo, false, nullptr, ETeleportType::TeleportPhysics);

	SetActorEnableCollision(true);
	SetCameraWide(false);

	AStairGameMode* GM = GetStairGameMode();
	UStairTerrain* Terrain = GM ? GM->GetTerrain() : nullptr;

	// ---- 着地点に足場が無い ＝ 落ちる ----
	// ★跳ぶ前に出した「落ちる予定」は使わない。着地するこの瞬間に見る。
	//
	//   音符を落とすと地形を詰めるので、跳んでいるあいだに足場が動く。
	//   跳ぶ前の判断を持ち越すと、着地点に足場ができていても
	//   予定どおり落ちてしまい、きれいに跳んだのに死ぬことがある。
	if (!Terrain || !Terrain->HasLandableStep(TargetRow, TargetLane))
	{
		// ★ここからは物理に任せる。
		//   跳躍だけを手で制御し、落下は素直に落とす。
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			const AStairGameMode* G = GetStairGameMode();
			const UStairConfig* Cfg = G ? G->GetConfig() : nullptr;
			Move->GravityScale = Cfg ? Cfg->JumpGravityScale : 3.f;

			// ★真下へ落とす。横の勢いを残さない。
			//   斜めに跳んで穴へ入ったとき、勢いが残っていると
			//   落ちながら横へ流れて、隣の足場に乗ってしまうことがあった。
			Move->Velocity = FVector::ZeroVector;
			Move->SetMovementMode(MOVE_Falling);
		}

		// 落ちたら持ち越した入力は捨てる
		bHasBuffered = false;

		// AnimBP に戻す。落下モーションは自動で出る
		RestoreAnimBlueprint();
		return;
	}

	// ---- 着地 ----
	bAirborne = false;
	CurrentRow = TargetRow;
	CurrentLane = TargetLane;

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->SetMovementMode(MOVE_Walking);
	}

	// 着地したので AnimBP に戻す
	RestoreAnimBlueprint();

	if (AStairStep* Step = Terrain->GetStep(CurrentRow, CurrentLane))
	{
		LaunchBaseZ = Step->GetActorLocation().Z;
		Step->OnStepped();
		OnLandedOnStep(CurrentRow);

		if (GM)
		{
			GM->NotifyLandedOn(Step);
		}
	}

	// ★跳んでいるあいだに押された入力を、ここで消化する。
	//   押した時刻のまま判定するので、遅れた扱いにはならない。
	if (bHasBuffered && GM)
	{
		const float Age = GM->GetSongTimeNow() - BufferedSongTime;
		const int32 Idx = BufferedNoteIndex;
		bHasBuffered = false;
		BufferedNoteIndex = -1;

		if (Age >= 0.f && Age <= InputBufferSeconds)
		{
			// 押した時点の音符に対して判定する
			ExecuteJump(BufferedDir, BufferedSongTime, Idx);
		}
	}
}

float AStairCharacter::ComputeFlightTime(bool bFromRed) const
{
	const AStairGameMode* GM = GetStairGameMode();
	const UStairConfig* C = GM ? GM->GetConfig() : nullptr;
	if (!C)
	{
		return 0.42f;
	}

	const float Window = GM ? GM->GetNextNoteWindow() : BIG_NUMBER;
	const bool bHasWindow = (Window < BIG_NUMBER * 0.5f);

	// ---- 赤マスは一拍ぶん浮いたままにする ----
	// ★すぐ着地せず、ゆっくり滞空させる。5段先まで跳ぶ大技だと分かるように。
	//   浮いているあいだは操作できない（着地するまで次を受け付けないため）。
	if (bFromRed)
	{
		float Red = C->JumpFlightTime;
		if (const UStairMusicClock* MC = GM ? GM->GetMusicClock() : nullptr)
		{
			Red = MC->GetBeatDuration() * FMath::Max(0.25f, C->RedFlightBeats);
		}

		// 次の音符を追い越さない範囲には収める
		if (bHasWindow)
		{
			Red = FMath::Min(Red, Window * C->JumpFlightGapRatio);
		}
		return FMath::Max(Red, C->MinJumpFlightTime);
	}

	// ★次の音符までに着地していないと、その音符は押せない。
	//   BPM200 の8分刻みは 300ms しかないので、
	//   0.42秒のまま跳ぶと譜面の大半が入力を受け付けなくなる。
	if (!bHasWindow)
	{
		return C->JumpFlightTime;   // 譜面が無い、または最後の音符
	}

	return FMath::Clamp(Window * C->JumpFlightGapRatio,
		FMath::Min(C->MinJumpFlightTime, C->JumpFlightTime), C->JumpFlightTime);
}

void AStairCharacter::TryJump(EStairDir InDir)
{
	if (!bControlEnabled)
	{
		return;
	}

	AStairGameMode* GM = GetStairGameMode();
	if (!GM)
	{
		return;
	}

	// ★跳んでいる最中でも入力を捨てない。着地した瞬間に跳ばせる。
	//   詰まった譜面では滞空時間が音符の間隔とほぼ同じなので、
	//   着地の直前に押された入力を捨てると跳べない音符が出る。
	if (bAirborne)
	{
		bHasBuffered = true;
		BufferedDir = InDir;
		BufferedSongTime = GM->GetSongTimeNow();

		// ★押した時点で狙っていた音符を覚えておく
		BufferedNoteIndex = GM->GetNextNoteIndex();
		return;
	}

	ExecuteJump(InDir, GM->GetSongTimeNow());
}

void AStairCharacter::ExecuteJump(EStairDir InDir, float PressSongTime, int32 NoteIndex)
{
	bHasBuffered = false;
	BufferedNoteIndex = -1;

	AStairGameMode* GM = GetStairGameMode();
	if (!GM)
	{
		return;
	}

	const UStairConfig* C = GM->GetConfig();
	UStairTerrain* Terrain = GM->GetTerrain();
	if (!C || !Terrain)
	{
		return;
	}

	// ---- リズム判定。押した瞬間の再生位置が基準 ----
	EStairJudge Judge = GM->JudgeAt(PressSongTime, NoteIndex);

	// ★向きが譜面と違えば、タイミングが良くても MISS。
	//   譜面どおりに叩くゲームなので、違う向きに跳んだら叩けていない。
	if (Judge != EStairJudge::Miss && !GM->DoesDirectionMatch(InDir, NoteIndex))
	{
		Judge = EStairJudge::Miss;
	}

	LastJudge = Judge;
	TimeSinceJudge = 0.f;
	GM->NotifyJudge(Judge,
		GM->GetSignedJudgeOffsetAt(PressSongTime, NoteIndex), NoteIndex);

	AStairStep* Here = Terrain->GetStep(CurrentRow, CurrentLane);
	const bool bFromRed = Here && Here->GetTile() == EStairTile::Red;

	// ---- ★次の音符に間に合う滞空時間を求める ----
	//   着地するまで次のジャンプは受け付けないので、
	//   音符が詰まっているところでは滞空時間を縮めないと押せなくなる。
	//   譜面が無いときは JumpFlightTime のまま。
	const float Flight = ComputeFlightTime(bFromRed);

	// ★短く跳ぶときは弧も低くする。
	//   時間だけ縮めると、同じ高さを一瞬で往復して針のように見える。
	const float ApexScale = FMath::Clamp(
		Flight / FMath::Max(0.01f, C->JumpFlightTime), 0.45f, 1.f);

	// ★赤は普段よりずっと高く跳ぶ。長距離ジャンプを見た目でも強調する
	const float Apex = bFromRed
		? C->RedJumpApex : (C->JumpApexClearance * ApexScale);

	// ---- MISS はその場ジャンプ ----
	if (Judge == EStairJudge::Miss)
	{
		LastSteps = 0;
		TargetRow = CurrentRow;
		TargetLane = CurrentLane;

		StartScriptedJump(GetStandLocation(CurrentRow, CurrentLane),
			Flight, C->MissJumpApex * ApexScale);
		PlayJumpSound(Judge);

		GM->NotifyMissJump();
		OnStairJumped(Judge, 0, bFromRed);
		return;
	}

	// ---- 進む段数を決める ----
	int32 StepsUp;
	if (bFromRed)
	{
		// 赤マスは PERFECT / GREAT どちらでも5段
		StepsUp = C->RedSteps;
	}
	else if (GM->HasChartRoad())
	{
		// ★譜面どおりの道を敷いているときは判定で距離を変えない。
		//   変えると着地点が二通りになり、道が定まらなくなる。
		//   判定はスコアとコンボにだけ効く。
		StepsUp = GM->GetStepsPerNote();
	}
	else
	{
		// ★エンドレスは道を敷いていないので、距離を変えてよい。
		//   PERFECT なら2段、GREAT なら1段。
		//   踏み外す危険と引き換えに伸びる、元からの駆け引きに戻す。
		StepsUp = (Judge == EStairJudge::Perfect) ? C->PerfectSteps : C->GreatSteps;
	}

	// ★押したキーがそのまま方向になる
	if (InDir != Dir)
	{
		Dir = InDir;
		OnDirChanged(Dir);
	}

	int32 LaneDelta = 0;
	if (Dir == EStairDir::Left)  { LaneDelta = -1; }
	if (Dir == EStairDir::Right) { LaneDelta = 1; }

	TargetRow = CurrentRow + StepsUp;
	TargetLane = CurrentLane + LaneDelta;
	LastSteps = StepsUp;

	// ---- ★通り道に壁があれば弾き返される ----
	//   着地点だけを見ていると、2段ジャンプが手前の壁をすり抜けてしまう。
	//   ただし赤マスの大跳躍だけは壁を飛び越えられる。
	bool bBlocked = Terrain->IsWall(TargetRow, TargetLane);

	// ★途中の段を見るのは「まっすぐ跳ぶとき」だけ。
	//   斜めジャンプは将棋の桂馬と同じで、正面の壁を回り込んで越えていく。
	//   ここで元のレーンまで見ていたため、正面が壁のときに
	//   左右へ避けても手前の斜めマスで止まってしまっていた。
	if (!bFromRed && !bBlocked && LaneDelta == 0)
	{
		for (int32 R = CurrentRow + 1; R < TargetRow; ++R)
		{
			if (Terrain->IsWall(R, CurrentLane))
			{
				bBlocked = true;
				break;
			}
		}
	}

	if (bBlocked)
	{
		// PERFECT で2段先が壁の場合、その手前（1段先）が空いていれば
		// そこへ着地させる。その場足踏みだと理不尽に感じるため。
		//
		// ★ただし譜面どおりの道を進んでいるときは手前で降ろさない。
		//   1段だけ進むと、この先の音符と足場の位置がずれてしまう。
		bool bRescued = false;

		if (StepsUp >= 2 && !GM->HasChartRoad())
		{
			for (int32 Back = 1; Back < StepsUp; ++Back)
			{
				const int32 R = CurrentRow + (StepsUp - Back);
				if (Terrain->HasLandableStep(R, TargetLane))
				{
					TargetRow = R;
					LastSteps = StepsUp - Back;
					bRescued = true;
					break;
				}
			}
		}

		if (!bRescued)
		{
			// 逃げ場が無い。跳ねて元の位置へ戻る
			LastJudge = EStairJudge::Miss;
			LastSteps = 0;
			TargetRow = CurrentRow;
			TargetLane = CurrentLane;

			StartScriptedJump(GetStandLocation(CurrentRow, CurrentLane),
				Flight, C->MissJumpApex * ApexScale);
			PlayJumpSound(EStairJudge::Miss);

			GM->NotifyWallBounce();
			OnStairJumped(EStairJudge::Miss, 0, bFromRed);
			return;
		}
	}

	// ---- 足場の補填 ----
	// ★譜面どおりの道があるときは補填しない。
	//   道の上には必ず足場があるので、緑が出るのは道を外れたときだけ。
	//   そこで助けてしまうと、譜面を無視しても進めてしまう。
	//   完全ランダムなエンドレスでは今までどおり補填する。
	const bool bNeedFill = !GM->HasChartRoad()
		&& (bFromRed || Judge == EStairJudge::Perfect);

	if (bNeedFill && !Terrain->IsWall(TargetRow, TargetLane))
	{
		Terrain->ForceCreateStep(TargetRow, TargetLane, EStairTile::Green);
	}

	// ---- 飛ばす ----
	// ★段数によらず滞空時間は同じ。5段跳びでもリズムがずれない。
	//   足場があるかどうかは着地するときに見るので、ここでは決めない。
	StartScriptedJump(GetStandLocation(TargetRow, TargetLane), Flight, Apex);
	PlayJumpSound(Judge);

	// 5段ジャンプはカメラを引く
	if (StepsUp >= C->RedSteps)
	{
		SetCameraWide(true);
	}

	// ★方向はリセットしない。押しっぱなしなら次も同じ方向へ行く
	OnStairJumped(Judge, StepsUp, bFromRed);
}

void AStairCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// ★着地の判断は FinishScriptedJump が行う。
	//   物理の接触に任せると、壁の側面に当たった時点で「着地」と
	//   みなされて中途半端な位置に立ってしまう。
	//   ここでは何もしない。
}
