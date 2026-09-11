#include "StairPlayerController.h"
#include "StairCharacter.h"
#include "StairGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"

void AStairPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (StairMappingContext)
			{
				Subsystem->AddMappingContext(StairMappingContext, 0);
			}
		}
	}
}

void AStairPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// ★A / D も押した瞬間に跳ぶ。SPACE と同じ扱い
		if (LeftAction)
		{
			EIC->BindAction(LeftAction, ETriggerEvent::Started,
				this, &AStairPlayerController::OnLeftPressed);
		}
		if (RightAction)
		{
			EIC->BindAction(RightAction, ETriggerEvent::Started,
				this, &AStairPlayerController::OnRightPressed);
		}
		if (JumpAction)
		{
			EIC->BindAction(JumpAction, ETriggerEvent::Started,
				this, &AStairPlayerController::OnJump);
		}
	}

	// ★譜面を打ち込むときだけ左クリックを使う。
	//   ゲーム操作としての射撃は廃止した。
	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed,
			this, &AStairPlayerController::OnFireKey);

		// ★Esc でポーズ。止まっている最中も効かないと解除できないので、
		//   bExecuteWhenPaused を立てる。
		FInputKeyBinding& B = InputComponent->BindKey(
			EKeys::Escape, IE_Pressed, this, &AStairPlayerController::OnPauseKey);
		B.bExecuteWhenPaused = true;

		// このコンポーネント自体も停止中に動くようにする
		bShouldPerformFullTickWhenPaused = true;
	}
}

bool AStairPlayerController::IsInputAllowed() const
{
	// ★Result中は入力を完全に遮断する。Playing 以外は一切受け付けない
	if (const AStairGameMode* GM = Cast<AStairGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		return GM->IsInputAllowed();
	}
	return false;
}

void AStairPlayerController::OnJump(const FInputActionValue& Value)
{
	DoJump(EStairDir::Forward);
}

void AStairPlayerController::OnFireKey()
{
	// 譜面編集中に音符を置く操作。それ以外では何もしない
	if (AStairGameMode* GM = Cast<AStairGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (GM->IsCharting())
		{
			GM->ChartPlace();
		}
	}
}

void AStairPlayerController::OnPauseKey()
{
	if (AStairGameMode* GM = Cast<AStairGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->TogglePause();
	}
}

void AStairPlayerController::OnLeftPressed(const FInputActionValue& Value)
{
	// ★押した瞬間に左へ跳ぶ。方向を溜めておく仕組みは廃止した
	DoJump(EStairDir::Left);
}

void AStairPlayerController::OnRightPressed(const FInputActionValue& Value)
{
	DoJump(EStairDir::Right);
}

void AStairPlayerController::DoJump(EStairDir InDir)
{
	AStairGameMode* GM = Cast<AStairGameMode>(UGameplayStatics::GetGameMode(this));

	// ★カウントダウン中でも押せる。
	//   START の瞬間に押せば、それが最初の1歩になる。
	//   まだ START に届いていなければ空押しとして拍だけ確かめられる。
	//
	//   ★曲が鳴り出すのを Tick で待っていたため、START と同時に押した
	//     1歩めが空押しとして捨てられていた。判定に間に合っていれば
	//     ここで遊びを始めて、そのまま跳ばせる。
	if (GM && GM->GetState() == EStairGameState::Countdown)
	{
		if (!GM->TryStartFromCountdown())
		{
			GM->NotifyPracticeTap();
			return;
		}
	}

	if (!IsInputAllowed()) { return; }
	if (AStairCharacter* C = Cast<AStairCharacter>(GetPawn()))
	{
		C->TryJump(InDir);
	}
}
