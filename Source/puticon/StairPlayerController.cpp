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
		// ★A / D は押しっぱなしを見る。押した瞬間と離した瞬間の両方を拾う
		if (LeftAction)
		{
			EIC->BindAction(LeftAction, ETriggerEvent::Started,
				this, &AStairPlayerController::OnLeftPressed);
			EIC->BindAction(LeftAction, ETriggerEvent::Completed,
				this, &AStairPlayerController::OnLeftReleased);
			EIC->BindAction(LeftAction, ETriggerEvent::Canceled,
				this, &AStairPlayerController::OnLeftReleased);
		}
		if (RightAction)
		{
			EIC->BindAction(RightAction, ETriggerEvent::Started,
				this, &AStairPlayerController::OnRightPressed);
			EIC->BindAction(RightAction, ETriggerEvent::Completed,
				this, &AStairPlayerController::OnRightReleased);
			EIC->BindAction(RightAction, ETriggerEvent::Canceled,
				this, &AStairPlayerController::OnRightReleased);
		}
		if (JumpAction)
		{
			EIC->BindAction(JumpAction, ETriggerEvent::Started,
				this, &AStairPlayerController::OnJump);
		}
	}

	// ★撃つ操作は入力アセットを作らず、キーを直接束ねる。
	//   Enter と左クリックの2つを同じ処理へ繋ぐ。
	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::Enter, IE_Pressed,
			this, &AStairPlayerController::OnFireKey);
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed,
			this, &AStairPlayerController::OnFireKey);
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
	AStairGameMode* GM = Cast<AStairGameMode>(UGameplayStatics::GetGameMode(this));

	// ★テンポ合わせの最中は、跳ばずに「押した」ことだけを数える
	if (GM && GM->GetState() == EStairGameState::Intro)
	{
		GM->NotifyIntroTap();
		return;
	}

	// ★カウントダウン中も押せるようにする。
	//   進みはしないが、拍を確かめ続けられる。
	if (GM && GM->GetState() == EStairGameState::Countdown)
	{
		GM->NotifyPracticeTap();
		return;
	}

	if (!IsInputAllowed()) { return; }
	if (AStairCharacter* C = Cast<AStairCharacter>(GetPawn()))
	{
		C->TryJump();
	}
}

void AStairPlayerController::OnFireKey()
{
	if (!IsInputAllowed()) { return; }
	if (AStairGameMode* GM = Cast<AStairGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->FireShot();
	}
}

void AStairPlayerController::OnLeftPressed(const FInputActionValue& Value)
{
	// 押しっぱなしの記録は Result 中でも消す必要があるので、
	// 状態チェックは押下時のみ行う
	if (!IsInputAllowed()) { return; }
	if (AStairCharacter* C = Cast<AStairCharacter>(GetPawn()))
	{
		C->SetHeldLeft(true);
	}
}

void AStairPlayerController::OnLeftReleased(const FInputActionValue& Value)
{
	// 離す側は常に受け付ける。押しっぱなしが残ると誤動作するため
	if (AStairCharacter* C = Cast<AStairCharacter>(GetPawn()))
	{
		C->SetHeldLeft(false);
	}
}

void AStairPlayerController::OnRightPressed(const FInputActionValue& Value)
{
	if (!IsInputAllowed()) { return; }
	if (AStairCharacter* C = Cast<AStairCharacter>(GetPawn()))
	{
		C->SetHeldRight(true);
	}
}

void AStairPlayerController::OnRightReleased(const FInputActionValue& Value)
{
	if (AStairCharacter* C = Cast<AStairCharacter>(GetPawn()))
	{
		C->SetHeldRight(false);
	}
}

