#include "StairMenuGameMode.h"
#include "StairWidgets.h"
#include "StairGameInstance.h"
#include "Camera/PlayerCameraManager.h"
#include "StairTerrain.h"
#include "StairStep.h"
#include "StairConfig.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "StairCharacter.h"
#include "Camera/CameraActor.h"
#include "GameFramework/SpectatorPawn.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void AStairMenuGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	// ★幕（UMG）が出そろうまでの数フレームを、カメラ側でも隠す。
	//   これが無いと、起動直後や切り替え直後に素の背景が一瞬映る。
	//   起動1回目だけは白、それ以外は丸に合わせて黒。
	if (PC->PlayerCameraManager)
	{
		bool bBoot = false;
		if (const UStairGameInstance* GI =
			Cast<UStairGameInstance>(GetGameInstance()))
		{
			// ★ウィジェットを作る前に読む。作ったあとだと消されている
			bBoot = !GI->bBootDone;
		}
		PC->PlayerCameraManager->SetManualCameraFade(
			1.f, bBoot ? FLinearColor::White : FLinearColor::Black, false);
		ScreenFadeHold = bBoot ? 0.35f : 0.1f;
	}

	if (MenuWidgetClass)
	{
		MenuWidget = CreateWidget<UUserWidget>(PC, MenuWidgetClass);
		if (MenuWidget)
		{
			MenuWidget->AddToViewport();
		}
	}

	UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(PC, MenuWidget, EMouseLockMode::DoNotLock);
	PC->bShowMouseCursor = true;

}

// =====================================================================
// タイトル：背景でステージを流す
// =====================================================================

AStairTitleGameMode::AStairTitleGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	MenuWidgetClass = UStairTitleWidget::StaticClass();
	Terrain = CreateDefaultSubobject<UStairTerrain>(TEXT("Terrain"));

	// ★DefaultPawn は球のメッシュを持っていて、開始地点にそのまま映る。
	//   タイトルは専用カメラで見せるので、見た目を持たないポーンにする。
	DefaultPawnClass = ASpectatorPawn::StaticClass();
}

void AStairTitleGameMode::BeginPlay()
{
	Super::BeginPlay();

	// ★Blueprint 側で DefaultPawn が指定されていると上の設定より優先される。
	//   球が映るのを確実に防ぐため、実体を隠しておく。
	if (APlayerController* PC0 = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (APawn* P = PC0->GetPawn())
		{
			P->SetActorHiddenInGame(true);
		}
	}

	// ★階段の始まりが映らないよう、少し進んだ位置から見せる
	ScrollRow = StartScrollRow;

	// ---- 背景のステージを敷く ----
	if (Terrain)
	{
		// ★タイトルでは最初の行を埋めない。板状の土台に見えるため
		Terrain->bSolidFirstRow = false;
		// ★背景は見せるだけなので穴を少なくする
		Terrain->HoleDensityOverride = TitleHoleDensity;
		// ★壁は出さない。背が高いので背景に柱が林立して階段が見えなくなる
		Terrain->WallChanceOverride = 0.f;
		Terrain->Initialize(Config);
		if (!Terrain->StepClass)
		{
			Terrain->StepClass = AStairStep::StaticClass();
		}
		// 開始位置の周りを先に敷いておく
		Terrain->UpdateAround(FMath::FloorToInt(ScrollRow), 0, 0.f);
	}

	// ---- 流し見用のカメラを置く ----
	UWorld* World = GetWorld();
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (World && PC)
	{
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ViewCamera = World->SpawnActor<ACameraActor>(
			ACameraActor::StaticClass(), FVector::ZeroVector, CameraRotation, SP);

		if (ViewCamera)
		{
			PC->SetViewTarget(ViewCamera);
		}
	}

	// ---- 見せ玉のキャラクターを並べる ----
	if (World)
	{
		UClass* Cls = DemoCharClass.Get();
		if (!Cls)
		{
			// ★C++ の AStairCharacter には見た目が無い。
			//   メッシュを持つのは BP_StairCharacter の方
			Cls = LoadClass<AStairCharacter>(nullptr,
				TEXT("/Game/Stair/Blueprints/BP_StairCharacter.BP_StairCharacter_C"));
		}
		if (!Cls)
		{
			Cls = AStairCharacter::StaticClass();
		}

		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// ★アニメーションを読み込む。テンプレート同梱のものを使う
		if (!DemoJumpAnim)
		{
			DemoJumpAnim = LoadObject<UAnimSequence>(nullptr,
				TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Jump.MM_Jump"));
		}
		if (!DemoFallAnim)
		{
			DemoFallAnim = LoadObject<UAnimSequence>(nullptr,
				TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Fall_Loop.MM_Fall_Loop"));
		}
		if (!DemoIdleAnim)
		{
			DemoIdleAnim = LoadObject<UAnimSequence>(nullptr,
				TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
		}

		// ★レーン範囲は Config に合わせる。指定があればそちらを優先
		if (DemoLaneSpread <= 0)
		{
			DemoLaneSpread = Config ? Config->LaneRadius : 7;
		}

		// レーンは1つ飛ばしで使うので、埋まる数はこれだけ
		const int32 LaneSlots = DemoLaneSpread + 1;

		const int32 N = FMath::Clamp(
			(DemoCount > 0) ? DemoCount : LaneSlots, 1, 80);

		DemoActors.Reset();
		Runners.Reset();

		for (int32 i = 0; i < N; ++i)
		{
			AStairCharacter* A = World->SpawnActor<AStairCharacter>(
				Cls, FVector(0, 0, 100000), FRotator::ZeroRotator, SP);
			if (!A)
			{
				continue;
			}

			// 見せるだけなので物理も当たり判定も切って手で動かす
			A->SetControlEnabled(false);
			if (UCharacterMovementComponent* Move = A->GetCharacterMovement())
			{
				Move->SetMovementMode(MOVE_None);
				Move->GravityScale = 0.f;
				Move->SetComponentTickEnabled(false);
			}
			A->SetActorEnableCollision(false);

			DemoActors.Add(A);
			Runners.Add(FDemoRunner());
		}

		// ★まとめて湧かせると重なって見えるので、1体ずつ順番に出す
		for (int32 i = 0; i < Runners.Num(); ++i)
		{
			Runners[i].bActive = false;
			Runners[i].SpawnDelay = i * FMath::Max(0.05f, DemoSpawnInterval);
			if (DemoActors[i])
			{
				DemoActors[i]->SetActorHiddenInGame(true);
			}
		}
	}


	// ★BGM はここでは鳴らさない。
	//   起動直後は背景の描画が追いつかず、白い幕で待つことになる。
	//   幕が明けるのに合わせて StartBGM を呼んでもらう。
}

bool AStairTitleGameMode::IsBackgroundReady() const
{
	const UStairConfig* C = Config;
	const float MaxWait = C ? C->BootMaxSeconds : 8.f;
	const float MinWait = C ? C->BootMinSeconds : 1.6f;

	// 保険。何があっても白のまま止まらないようにする
	if (BackgroundWait >= MaxWait)
	{
		return true;
	}

	// 段がまだ生成されていない
	if (!Terrain || Terrain->GetStepCount() <= 0)
	{
		return false;
	}

	// ★段があっても、マテリアルの準備が済むまでは描画されない。
	//   その完了をゲームから正しく知る手立てが無いので、
	//   最低限の時間を置いて待つ。
	return BackgroundWait >= MinWait;
}

void AStairTitleGameMode::StartBGM()
{
	if (BGMComp || !Config || !Config->TitleBGM)
	{
		return;
	}

	BGMComp = UGameplayStatics::SpawnSound2D(
		this, Config->TitleBGM, Config->TitleBGMVolume, 1.f, 0.f,
		nullptr, false, true);
	if (BGMComp)
	{
		// 音源側の Looping 設定に頼らず、ここでも確実にループさせる
		BGMComp->bAutoDestroy = false;
		BGMComp->OnAudioFinished.AddDynamic(
			this, &AStairTitleGameMode::HandleBGMFinished);
	}
}

void AStairMenuGameModeBase::TickScreenFade(float DeltaSeconds)
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

	// 幕が出そろったので、カメラ側の覆いは解く
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StopCameraFade();
		}
	}
}

void AStairTitleGameMode::SetBGMPaused(bool bPause)
{
	if (BGMComp)
	{
		BGMComp->SetPaused(bPause);
	}
}

float AStairTitleGameMode::GetStandZOffset() const
{
	// 段の厚みの半分 ＋ カプセルの半分
	const float HalfStep = Config ? Config->StepScale.Z * 100.f * 0.5f : 22.5f;
	float HalfCapsule = 88.f;
	if (DemoActors.Num() > 0 && DemoActors[0])
	{
		if (const UCapsuleComponent* Cap = DemoActors[0]->GetCapsuleComponent())
		{
			HalfCapsule = Cap->GetScaledCapsuleHalfHeight();
		}
	}
	return HalfStep + HalfCapsule + 2.f;
}

void AStairTitleGameMode::PlayDemoAnim(int32 Index, int32 Kind)
{
	if (!DemoActors.IsValidIndex(Index) || !DemoActors[Index])
	{
		return;
	}
	USkeletalMeshComponent* M = DemoActors[Index]->GetMesh();
	if (!M)
	{
		return;
	}

	UAnimSequence* Anim = nullptr;
	bool bLoop = false;

	switch (Kind)
	{
	case 0: Anim = DemoIdleAnim; bLoop = true;  break;   // 待機
	case 1: Anim = DemoJumpAnim; bLoop = false; break;   // 跳躍
	default: Anim = DemoFallAnim; bLoop = true; break;   // 落下
	}

	if (!Anim)
	{
		return;
	}

	// PlayAnimation は AnimBP を切って単体再生に切り替える
	M->PlayAnimation(Anim, bLoop);
	M->SetPlayRate(1.f);

	if (Kind == 1 && Runners.IsValidIndex(Index))
	{
		// 跳躍時間ぴったりで1周するよう速さを合わせる
		const float Len = Anim->GetPlayLength();
		const float Hop = FMath::Max(0.05f, Runners[Index].HopTime);
		if (Len > 0.f)
		{
			M->SetPlayRate(FMath::Clamp(Len / Hop, 0.25f, 4.f));
		}
	}
}

bool AStairTitleGameMode::IsCellTaken(int32 Row, int32 Lane, int32 SkipIndex) const
{
	for (int32 i = 0; i < Runners.Num(); ++i)
	{
		if (i == SkipIndex)
		{
			continue;
		}
		const FDemoRunner& O = Runners[i];
		if (!O.bActive || O.bFalling)
		{
			continue;
		}
		// いま乗っているマスと、跳び先のマスを塞がっているとみなす
		if ((O.FromRow == Row && O.FromLane == Lane)
			|| (O.ToRow == Row && O.ToLane == Lane))
		{
			return true;
		}
	}
	return false;
}

void AStairTitleGameMode::ScheduleRespawn(int32 Index, float Delay)
{
	if (!Runners.IsValidIndex(Index))
	{
		return;
	}
	Runners[Index].bActive = false;
	Runners[Index].SpawnDelay = FMath::Max(0.05f, Delay);
	if (DemoActors.IsValidIndex(Index) && DemoActors[Index])
	{
		DemoActors[Index]->SetActorHiddenInGame(true);
	}
}

void AStairTitleGameMode::RespawnRunner(int32 Index)
{
	if (!Runners.IsValidIndex(Index) || !Terrain)
	{
		return;
	}
	FDemoRunner& R = Runners[Index];

	const int32 Base = FMath::FloorToInt(ScrollRow);

	// ★レーンは端から順に一巡させる。全レーンに行き渡らせるため。
	//   1つ飛ばしで使うので、隣り合って並ぶことはない。
	const int32 LaneCount = DemoLaneSpread + 1;   // -S, -S+2, ... , +S
	int32 Row = Base + DemoSpawnRowMin;
	int32 Lane = 0;
	bool bFound = false;

	for (int32 Step = 0; Step < LaneCount && !bFound; ++Step)
	{
		const int32 Slot = (NextSpawnLane + Step) % LaneCount;
		const int32 L2 = -DemoLaneSpread + Slot * 2;

		// そのレーンで、足場があって空いている段を探す
		for (int32 Try = 0; Try < 10; ++Try)
		{
			const int32 R2 = Base + FMath::RandRange(DemoSpawnRowMin, DemoSpawnRowMax);

			if (!Terrain->HasStep(R2, L2))
			{
				continue;   // 穴の上には出さない
			}
			if (IsCellTaken(R2, L2, Index))
			{
				continue;   // 他の個体と重ならない
			}

			Row = R2;
			Lane = L2;
			bFound = true;
			// 次はその隣のレーンから始める
			NextSpawnLane = (NextSpawnLane + Step + 1) % LaneCount;
			break;
		}
	}

	if (!bFound)
	{
		// 空きが無ければ少し待ってから出し直す
		ScheduleRespawn(Index, 0.4f);
		return;
	}

	R.bActive = true;
	R.SpawnDelay = 0.f;
	if (DemoActors.IsValidIndex(Index) && DemoActors[Index])
	{
		DemoActors[Index]->SetActorHiddenInGame(false);
	}

	R.FromRow = Row;
	R.FromLane = Lane;
	R.ToRow = Row;
	R.ToLane = Lane;

	R.bFalling = false;
	R.HopT = 0.f;
	R.Dodges = 0;
	R.HopTime = DemoHopTime * FMath::FRandRange(0.85f, 1.25f);
	R.WaitLeft = FMath::FRandRange(DemoWaitMin, DemoWaitMax);

	PlayDemoAnim(Index, 0);
}

void AStairTitleGameMode::PickNextHop(int32 Index)
{
	if (!Runners.IsValidIndex(Index) || !Terrain)
	{
		return;
	}
	FDemoRunner& R = Runners[Index];

	R.HopT = 0.f;

	const int32 NextRow = R.FromRow + 1;
	const int32 Limit = DemoLaneSpread + 1;

	// その場所が「穴だと確定している」か。
	// まだ生成されていない前方は穴とみなさない（生成待ちで落ちてしまうため）
	auto IsHole = [&](int32 Lane)
	{
		return Terrain->IsGenerated(NextRow, Lane)
			&& !Terrain->HasStep(NextRow, Lane);
	};

	// 基本はまっすぐ
	R.ToRow = NextRow;
	R.ToLane = R.FromLane;

	if (IsHole(R.FromLane))
	{
		// ★避けるかどうか自体がランダム。避けなければそのまま落ちる。
		//   避けられるのは DemoMaxDodges 回まで。
		const bool bTryDodge =
			(R.Dodges < DemoMaxDodges) && (FMath::FRand() < DemoDodgeChance);

		if (bTryDodge)
		{
			++R.Dodges;

			// 左右どちらへ逃げるかもランダムに1回決めるだけ。
			// 逃げた先が穴でも構わず跳んで、そのまま落ちる。
			const int32 Side = FMath::RandBool() ? 1 : -1;
			const int32 L = R.FromLane + Side;
			if (FMath::Abs(L) <= Limit)
			{
				R.ToLane = L;
			}
		}
	}

	// 跳び先が穴なら落ちる
	if (IsHole(R.ToLane))
	{
		R.bFalling = true;
		R.FallPos = Terrain->GetStepLocation(R.FromRow, R.FromLane)
			+ FVector(0.f, 0.f, GetStandZOffset());

		// 跳んだ勢いのまま穴へ落ちていく
		const FVector Aim = Terrain->GetStepLocation(R.ToRow, R.ToLane);
		FVector Dir = Aim - R.FallPos;
		Dir.Z = 0.f;
		Dir.Normalize();

		R.FallVel = Dir * 380.f + FVector(0.f, 0.f, 480.f);
		PlayDemoAnim(Index, 2);
	}
	else
	{
		PlayDemoAnim(Index, 1);
	}
}

void AStairTitleGameMode::UpdateDemoRunners(float DeltaSeconds)
{
	if (!Terrain)
	{
		return;
	}

	for (int32 i = 0; i < Runners.Num(); ++i)
	{
		if (!DemoActors.IsValidIndex(i) || !DemoActors[i])
		{
			continue;
		}
		AStairCharacter* A = DemoActors[i];
		FDemoRunner& R = Runners[i];

		// ---------- 出現待ち ----------
		// ★まとめて湧かないよう、順番が来るまで隠しておく
		if (!R.bActive)
		{
			R.SpawnDelay -= DeltaSeconds;
			if (R.SpawnDelay <= 0.f)
			{
				RespawnRunner(i);
			}
			continue;
		}

		// ---------- 落下中 ----------
		if (R.bFalling)
		{
			R.FallVel.Z -= 2600.f * DeltaSeconds;
			R.FallPos += R.FallVel * DeltaSeconds;
			A->SetActorLocation(R.FallPos, false, nullptr,
				ETeleportType::TeleportPhysics);
			A->SetActorRotation(FRotator(0.f, R.Yaw, 0.f));

			const float Ground = Terrain->GetStepLocation(
				FMath::FloorToInt(ScrollRow), 0).Z;
			if (R.FallPos.Z < Ground - 2200.f)
			{
				// 落ちきったら少し間をおいてから出し直す
				ScheduleRespawn(i, FMath::FRandRange(0.3f, 1.2f));
			}
			continue;
		}

		// ---------- 着地して休んでいる ----------
		if (R.WaitLeft > 0.f)
		{
			R.WaitLeft -= DeltaSeconds;

			FVector P = Terrain->GetStepLocation(R.FromRow, R.FromLane);
			P.Z += GetStandZOffset();
			A->SetActorLocation(P, false, nullptr, ETeleportType::TeleportPhysics);
			A->SetActorRotation(FRotator(0.f, R.Yaw, 0.f));

			if (R.WaitLeft <= 0.f)
			{
				PickNextHop(i);
			}
			continue;
		}

		// ---------- 跳躍中 ----------
		R.HopT += DeltaSeconds / FMath::Max(0.05f, R.HopTime);

		if (R.HopT >= 1.f)
		{
			// 着地。次の出発点にして、ひと呼吸おく
			R.FromRow = R.ToRow;
			R.FromLane = R.ToLane;
			R.HopT = 0.f;
			R.HopTime = DemoHopTime * FMath::FRandRange(0.85f, 1.25f);
			R.WaitLeft = FMath::FRandRange(DemoWaitMin, DemoWaitMax);
			PlayDemoAnim(i, 0);

			// 画面から外れていたら間をおいて出し直す
			const float Ahead = R.FromRow - ScrollRow;
			if (Ahead > 26.f || Ahead < -16.f)
			{
				ScheduleRespawn(i, FMath::FRandRange(0.3f, 1.2f));
			}
			continue;
		}

		const FVector From = Terrain->GetStepLocation(R.FromRow, R.FromLane);
		const FVector To = Terrain->GetStepLocation(R.ToRow, R.ToLane);

		FVector P = FMath::Lerp(From, To, R.HopT);
		P.Z += GetStandZOffset() + FMath::Sin(PI * R.HopT) * DemoHopHeight;
		A->SetActorLocation(P, false, nullptr, ETeleportType::TeleportPhysics);

		// 常に進行方向を向く。ピッチは付けない（倒れて見えるため）
		FVector Dir = To - From;
		Dir.Z = 0.f;
		if (!Dir.IsNearlyZero())
		{
			R.Yaw = Dir.Rotation().Yaw;
		}
		A->SetActorRotation(FRotator(0.f, R.Yaw, 0.f));
	}
}

void AStairTitleGameMode::HandleBGMFinished()
{
	// 鳴り終わったら頭から鳴らし直す
	if (BGMComp)
	{
		BGMComp->Play(0.f);
	}
}

void AStairTitleGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
	if (BGMComp)
	{
		BGMComp->OnAudioFinished.RemoveDynamic(
			this, &AStairTitleGameMode::HandleBGMFinished);
		BGMComp->Stop();
		BGMComp = nullptr;
	}
	Super::EndPlay(Reason);
}

void AStairTitleGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 起動直後・切り替え直後の覆いを、幕が出そろったら解く
	TickScreenFade(DeltaSeconds);

	// 白い幕を明けてよいかの判断に使う
	BackgroundWait += DeltaSeconds;

	if (!Terrain || !ViewCamera)
	{
		return;
	}

	// ゆっくり登り続ける。
	// ★1フレームの進みに上限を置く。起動直後は読み込みで数秒詰まることがあり、
	//   そのぶんをまとめて進めるとカメラが一気に飛んで、
	//   まだ敷けていない先へ出てしまう（背景の階段が消えて見える）。
	ScrollRow += ScrollRowsPerSecond * FMath::Min(DeltaSeconds, 0.05f);

	const int32 Row = FMath::FloorToInt(ScrollRow);
	Terrain->UpdateAround(Row, 0, 0.f);

	// 段と段のあいだを補間して滑らかに動かす
	const float Frac = ScrollRow - Row;
	const FVector A = Terrain->GetStepLocation(Row, 0);
	const FVector B = Terrain->GetStepLocation(Row + 1, 0);
	const FVector Base = FMath::Lerp(A, B, Frac);

	ViewCamera->SetActorLocation(Base + CameraOffset);
	ViewCamera->SetActorRotation(CameraRotation);

	UpdateDemoRunners(DeltaSeconds);
}

AStairBGMSelectGameMode::AStairBGMSelectGameMode()
{
	MenuWidgetClass = UStairBGMSelectWidget::StaticClass();
}
