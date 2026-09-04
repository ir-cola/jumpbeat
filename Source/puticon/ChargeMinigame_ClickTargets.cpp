#include "ChargeMinigame_ClickTargets.h"
#include "DanGameParams.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"

FText UChargeMinigame_ClickTargets::GetDisplayName() const
{
	return FText::FromString(TEXT("飛来球"));
}

EChargeMinigameType UChargeMinigame_ClickTargets::GetMinigameType() const
{
	return EChargeMinigameType::ClickTargets;
}

float UChargeMinigame_ClickTargets::GetProgress() const
{
	if (!Params || Params->ClickDuration <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(Elapsed / Params->ClickDuration, 0.f, 1.f);
}

void UChargeMinigame_ClickTargets::BeginMinigame()
{
	Super::BeginMinigame();

	Elapsed = 0.f;
	CaughtCount = 0;

	if (GEngine && GEngine->GameViewport)
	{
		FVector2D Size;
		GEngine->GameViewport->GetViewportSize(Size);
		if (Size.X > 0.f && Size.Y > 0.f)
		{
			ViewportSize = Size;
		}
	}

	BuildTargets();
}

void UChargeMinigame_ClickTargets::BuildTargets()
{
	Targets.Reset();

	if (!Params)
	{
		return;
	}

	const int32 Blue = FMath::Max(1, Params->ClickBlueCount);
	const int32 Red  = FMath::Max(0, Params->ClickRedCount);
	const int32 Total = Blue + Red;

	// 出現時刻を均等に散らし、最後の球が制限時間内に取れるようにする
	const float LastSpawn = FMath::Max(0.1f, Params->ClickDuration - 0.6f);

	for (int32 i = 0; i < Total; ++i)
	{
		FDanFlyingTarget T;
		T.bIsRed = (i >= Blue);
		T.SpawnTime = (Total > 1) ? (LastSpawn * i / (Total - 1)) : 0.f;
		T.TravelTime = FMath::FRandRange(1.0f, 1.6f);

		// 画面の左右どちらかの外から入ってきて、反対側の外へ抜ける
		const bool bFromLeft = FMath::RandBool();
		const float Y0 = FMath::FRandRange(ViewportSize.Y * 0.15f, ViewportSize.Y * 0.80f);
		const float Y1 = FMath::FRandRange(ViewportSize.Y * 0.15f, ViewportSize.Y * 0.80f);
		const float Margin = 120.f;

		T.StartPos = bFromLeft
			? FVector2D(-Margin, Y0)
			: FVector2D(ViewportSize.X + Margin, Y0);
		T.EndPos = bFromLeft
			? FVector2D(ViewportSize.X + Margin, Y1)
			: FVector2D(-Margin, Y1);

		Targets.Add(T);
	}

	// 赤球が最後にまとまらないよう順番をシャッフルする
	for (int32 i = Targets.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		const float SpawnA = Targets[i].SpawnTime;
		const float SpawnB = Targets[j].SpawnTime;
		Targets.Swap(i, j);
		// 出現時刻だけは元の並び（均等配置）を保つ
		Targets[i].SpawnTime = SpawnA;
		Targets[j].SpawnTime = SpawnB;
	}
}

FVector2D UChargeMinigame_ClickTargets::GetTargetPosition(int32 Index, bool& bVisible) const
{
	bVisible = false;

	if (!Targets.IsValidIndex(Index))
	{
		return FVector2D::ZeroVector;
	}

	const FDanFlyingTarget& T = Targets[Index];
	if (T.bTaken)
	{
		return FVector2D::ZeroVector;
	}

	const float Local = Elapsed - T.SpawnTime;
	if (Local < 0.f || Local > T.TravelTime)
	{
		return FVector2D::ZeroVector;
	}

	bVisible = true;
	const float Alpha = FMath::Clamp(Local / T.TravelTime, 0.f, 1.f);
	return FMath::Lerp(T.StartPos, T.EndPos, Alpha);
}

void UChargeMinigame_ClickTargets::OnClickAt(FVector2D ScreenPos)
{
	if (bFinished || !Params)
	{
		return;
	}

	// 一番近い「いま画面にいる球」を探す
	int32 BestIndex = INDEX_NONE;
	float BestDistSq = Params->ClickRadius * Params->ClickRadius;

	for (int32 i = 0; i < Targets.Num(); ++i)
	{
		bool bVisible = false;
		const FVector2D Pos = GetTargetPosition(i, bVisible);
		if (!bVisible)
		{
			continue;
		}

		const float DistSq = FVector2D::DistSquared(Pos, ScreenPos);
		if (DistSq <= BestDistSq)
		{
			BestDistSq = DistSq;
			BestIndex = i;
		}
	}

	if (BestIndex == INDEX_NONE)
	{
		return; // 空振り。ペナルティなし
	}

	Targets[BestIndex].bTaken = true;
	OnTargetCaught(BestIndex, Targets[BestIndex].bIsRed);

	// 赤球を掴んだら即暴発
	if (Targets[BestIndex].bIsRed)
	{
		TriggerBurst();
		return;
	}

	++CaughtCount;
	Charge = FMath::Clamp((float)CaughtCount / (float)FMath::Max(1, Params->ClickBlueCount), 0.f, 1.f);
	UpdateDanAndNotify();

	// 青球を全部取ったら即終了
	if (CaughtCount >= Params->ClickBlueCount)
	{
		EndMinigame();
	}
}

void UChargeMinigame_ClickTargets::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bFinished || !Params)
	{
		return;
	}

	Elapsed += DeltaTime;

	if (Elapsed >= Params->ClickDuration)
	{
		EndMinigame();
	}
}
