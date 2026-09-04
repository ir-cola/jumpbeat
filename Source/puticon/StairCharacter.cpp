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

void AStairCharacter::SetHeldLeft(bool bHeld)
{
	bHeldLeft = bHeld;
	UpdateDirFromHeld();
}

void AStairCharacter::SetHeldRight(bool bHeld)
{
	bHeldRight = bHeld;
	UpdateDirFromHeld();
}

void AStairCharacter::UpdateDirFromHeld()
{
	// 両方押し・どちらも押していない → 正面
	EStairDir NewDir = EStairDir::Forward;
	if (bHeldLeft && !bHeldRight)  { NewDir = EStairDir::Left; }
	if (bHeldRight && !bHeldLeft)  { NewDir = EStairDir::Right; }

	if (NewDir != Dir)
	{
		Dir = NewDir;
		OnDirChanged(Dir);
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
	float ApexClearance, bool bFallAfter)
{
	JumpFrom = GetActorLocation();
	JumpTo = Target;
	JumpElapsed = 0.f;
	JumpDuration = FMath::Max(0.12f, FlightTime);
	bFallOnLand = bFallAfter;

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
	if (bFallOnLand || !Terrain || !Terrain->HasLandableStep(TargetRow, TargetLane))
	{
		// ★ここからは物理に任せる。
		//   跳躍だけを手で制御し、落下は素直に落とす。
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			const AStairGameMode* G = GetStairGameMode();
			const UStairConfig* Cfg = G ? G->GetConfig() : nullptr;
			Move->GravityScale = Cfg ? Cfg->JumpGravityScale : 3.f;

			// 跳んできた勢いを残して落とす
			Move->Velocity = FVector(
				(JumpTo.X - JumpFrom.X) / JumpDuration,
				(JumpTo.Y - JumpFrom.Y) / JumpDuration,
				0.f);
			Move->SetMovementMode(MOVE_Falling);
		}

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
}

void AStairCharacter::TryJump()
{
	if (!bControlEnabled || bAirborne)
	{
		return;
	}

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

	// ---- リズム判定。オーディオ再生位置が基準 ----
	const EStairJudge Judge = GM->JudgeNow();
	LastJudge = Judge;
	TimeSinceJudge = 0.f;
	GM->NotifyJudge(Judge);

	AStairStep* Here = Terrain->GetStep(CurrentRow, CurrentLane);
	const bool bFromRed = Here && Here->GetTile() == EStairTile::Red;

	// ---- MISS はその場ジャンプ ----
	if (Judge == EStairJudge::Miss)
	{
		LastSteps = 0;
		TargetRow = CurrentRow;
		TargetLane = CurrentLane;

		// MISS でも滞空時間は同じ。リズムを崩さないため
		StartScriptedJump(GetStandLocation(CurrentRow, CurrentLane),
			C->JumpFlightTime, C->MissJumpApex, false);
		PlayJumpSound(Judge);

		GM->NotifyMissOnCurrentStep();
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
	else
	{
		StepsUp = (Judge == EStairJudge::Perfect) ? C->PerfectSteps : C->GreatSteps;
	}

	// ★ジャンプする瞬間の押しっぱなし状態で方向を決める
	UpdateDirFromHeld();

	int32 LaneDelta = 0;
	if (Dir == EStairDir::Left)  { LaneDelta = -1; }
	if (Dir == EStairDir::Right) { LaneDelta = 1; }

	TargetRow = CurrentRow + StepsUp;
	TargetLane = CurrentLane + LaneDelta;
	LastSteps = StepsUp;

	// ---- ★着地点が壁のときだけ弾き返される ----
	//   途中に壁があっても関係ない。位置を直接動かすので跳び越えられる。
	//   赤マスの5段ジャンプが手前の壁で止まらないのはこのため。
	if (Terrain->IsWall(TargetRow, TargetLane))
	{
		// PERFECT で2段先が壁の場合、その手前（1段先）が空いていれば
		// そこへ着地させる。その場足踏みだと理不尽に感じるため。
		bool bRescued = false;

		if (StepsUp >= 2)
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
				C->JumpFlightTime, C->MissJumpApex, false);
			PlayJumpSound(EStairJudge::Miss);

			GM->NotifyWallBounce();
			OnStairJumped(EStairJudge::Miss, 0, bFromRed);
			return;
		}
	}

	// ---- 足場の補填 ----
	// ★ジャスト入力を解決したこの瞬間（滞空に入る直前）に生成する
	const bool bNeedFill =
		bFromRed                             // 赤マス: 5段＋足場生成
		|| (Judge == EStairJudge::Perfect);  // PERFECT: 2段＋緑の補填足場

	if (bNeedFill && !Terrain->IsWall(TargetRow, TargetLane))
	{
		Terrain->ForceCreateStep(TargetRow, TargetLane, EStairTile::Green);
	}

	// ---- 飛ばす ----
	// ★段数によらず滞空時間は同じ。5段跳びでもリズムがずれない。
	//   着地点に足場が無ければ、着いたあと落ちる。
	const bool bWillFall = !Terrain->HasLandableStep(TargetRow, TargetLane);

	StartScriptedJump(GetStandLocation(TargetRow, TargetLane),
		C->JumpFlightTime, C->JumpApexClearance, bWillFall);
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
