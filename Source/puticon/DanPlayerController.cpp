#include "DanPlayerController.h"
#include "DanGameMode.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"

ADanPlayerController::ADanPlayerController()
{
}

void ADanPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// ★これを呼ばないと入力が一切来ない（UE5で最も引っかかる箇所）
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (DanMappingContext)
			{
				Subsystem->AddMappingContext(DanMappingContext, 0);
			}
		}
	}
}

void ADanPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (ActionInput)
		{
			EIC->BindAction(ActionInput, ETriggerEvent::Started,
				this, &ADanPlayerController::OnActionPressed);
			EIC->BindAction(ActionInput, ETriggerEvent::Completed,
				this, &ADanPlayerController::OnActionReleased);
		}
		if (PointerInput)
		{
			EIC->BindAction(PointerInput, ETriggerEvent::Triggered,
				this, &ADanPlayerController::OnPointerMoved);
		}
	}
}

void ADanPlayerController::OnActionPressed(const FInputActionValue& Value)
{
	ADanGameMode* GM = Cast<ADanGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM)
	{
		return;
	}

	// 飛来球ミニゲーム用に、押した瞬間のマウス座標も渡す
	float MouseX = 0.f, MouseY = 0.f;
	if (GetMousePosition(MouseX, MouseY))
	{
		GM->HandleClickAt(FVector2D(MouseX, MouseY));
	}

	GM->HandleActionPressed();
}

void ADanPlayerController::OnActionReleased(const FInputActionValue& Value)
{
	if (ADanGameMode* GM = Cast<ADanGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->HandleActionReleased();
	}
}

void ADanPlayerController::OnPointerMoved(const FInputActionValue& Value)
{
	if (ADanGameMode* GM = Cast<ADanGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->HandlePointerMoved(Value.Get<FVector2D>());
	}
}
