#include "StairTerrain.h"
#include "StairStep.h"
#include "StairConfig.h"
#include "Engine/World.h"

UStairTerrain::UStairTerrain()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStairTerrain::Initialize(UStairConfig* InConfig)
{
	Config = InConfig;
	ClearAll();
}

void UStairTerrain::ClearAll()
{
	for (const TPair<int64, TObjectPtr<AStairStep>>& P : Steps)
	{
		if (P.Value)
		{
			P.Value->Destroy();
		}
	}
	Steps.Empty();
	Holes.Empty();
	RowRange.Empty();
	GeneratedUpTo = -1;
	LastRedRow = -1000;
}

FVector UStairTerrain::GetStepLocation(int32 Row, int32 Lane) const
{
	const float H = Config ? Config->StepHeight : 62.f;
	const float D = Config ? Config->StepDepth : 230.f;
	const float W = Config ? Config->LaneWidth : 260.f;
	return FVector(Row * D, Lane * W, Row * H);
}

bool UStairTerrain::HasStep(int32 Row, int32 Lane) const
{
	return Steps.Contains(Key(Row, Lane));
}

AStairStep* UStairTerrain::GetStep(int32 Row, int32 Lane) const
{
	const TObjectPtr<AStairStep>* Found = Steps.Find(Key(Row, Lane));
	return Found ? Found->Get() : nullptr;
}

bool UStairTerrain::IsKnownHole(int32 Row, int32 Lane) const
{
	return Holes.Contains(Key(Row, Lane));
}

AStairStep* UStairTerrain::SpawnStep(int32 Row, int32 Lane, EStairTile Tile)
{
	UWorld* World = GetWorld();
	if (!World || !StepClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AStairStep* Step = World->SpawnActor<AStairStep>(
		StepClass, GetStepLocation(Row, Lane), FRotator::ZeroRotator, SP);
	if (!Step)
	{
		return nullptr;
	}

	Step->Setup(Row, Lane, Tile);
	ApplyStepTransform(Step, Row, Lane, Tile);

	Steps.Add(Key(Row, Lane), Step);
	Holes.Remove(Key(Row, Lane));
	return Step;
}

void UStairTerrain::ApplyStepTransform(AStairStep* Step, int32 Row, int32 Lane,
	EStairTile Tile) const
{
	if (!Step || !Config)
	{
		return;
	}

	FVector Scale = Config->StepScale;
	FVector Loc = GetStepLocation(Row, Lane);

	if (Tile == EStairTile::Wall)
	{
		// ★壁の上面を「1段上の足場」と同じ高さに揃える。
		//   こうすると上の段から見たとき、同じ高さに床が続いて見える。
		//   下の段からは登れない厚い壁になる。
		//
		//     厚み = 通常の厚み ＋ 段の高さ1つぶん
		//     底面は動かさないので、中心は増えたぶんの半分だけ上げる
		const float Thick = Config->StepScale.Z * 100.f;
		const float Extra = Config->StepHeight;

		Scale.Z = (Thick + Extra) / 100.f;
		Loc.Z += Extra * 0.5f;
	}

	Step->SetActorScale3D(Scale);
	Step->SetActorLocation(Loc);
}

AStairStep* UStairTerrain::ForceCreateStep(int32 Row, int32 Lane, EStairTile Tile)
{
	if (AStairStep* Existing = GetStep(Row, Lane))
	{
		return Existing;
	}

	AStairStep* S = SpawnStep(Row, Lane, Tile);

	// ★補填で生えた足場は下から浮き上がらせる。
	//   いきなり現れると「元からあった」ように見えてしまう。
	if (S)
	{
		S->StartRise();
	}
	return S;
}

void UStairTerrain::RemoveStep(int32 Row, int32 Lane)
{
	const int64 K = Key(Row, Lane);
	if (TObjectPtr<AStairStep>* Found = Steps.Find(K))
	{
		if (*Found)
		{
			(*Found)->Destroy();
		}
		Steps.Remove(K);
		Holes.Add(K);
	}
}

// =====================================================================
// 制約チェック
// =====================================================================

bool UStairTerrain::IsWall(int32 Row, int32 Lane) const
{
	if (const TObjectPtr<AStairStep>* Found = Steps.Find(Key(Row, Lane)))
	{
		return (*Found) && (*Found)->GetTile() == EStairTile::Wall;
	}
	return false;
}

bool UStairTerrain::WouldExceedHorizontalRun(int32 Row, int32 Lane) const
{
	// ★穴と壁を合算して数える。
	//   壁も「通れないマス」なので、穴の隣に壁が並ぶと2連続になってしまう。
	const int32 MaxRun = Config ? Config->MaxBlockedRun : 1;

	int32 Run = 1;
	for (int32 L = Lane - 1; IsBlocked(Row, L); --L)
	{
		++Run;
		if (Run > MaxRun) { return true; }
	}
	for (int32 L = Lane + 1; IsBlocked(Row, L); ++L)
	{
		++Run;
		if (Run > MaxRun) { return true; }
	}
	return Run > MaxRun;
}

bool UStairTerrain::WouldExceedCluster(int32 Row, int32 Lane) const
{
	const int32 MaxCluster = Config ? Config->MaxHoleCluster : 4;

	// この位置を穴と仮定して、上下左右に連結した穴の数を数える
	TSet<int64> Visited;
	TArray<TPair<int32, int32>> Stack;

	Visited.Add(Key(Row, Lane));
	Stack.Add(TPair<int32, int32>(Row, Lane));

	int32 Count = 0;

	while (Stack.Num() > 0)
	{
		const TPair<int32, int32> Cur = Stack.Pop();
		++Count;
		if (Count > MaxCluster)
		{
			return true;
		}

		const int32 DR[4] = { 1, -1, 0, 0 };
		const int32 DL[4] = { 0, 0, 1, -1 };
		for (int32 i = 0; i < 4; ++i)
		{
			const int32 NR = Cur.Key + DR[i];
			const int32 NL = Cur.Value + DL[i];
			const int64 NK = Key(NR, NL);

			if (Visited.Contains(NK))
			{
				continue;
			}
			if (!IsKnownHole(NR, NL))
			{
				continue;
			}
			Visited.Add(NK);
			Stack.Add(TPair<int32, int32>(NR, NL));
		}
	}

	return Count > MaxCluster;
}

void UStairTerrain::EnsureReachability(int32 Row, int32 MinLane, int32 MaxLane)
{
	// 前の行の各足場について、正面・斜め左・斜め右のどれかが足場であることを保証する
	const int32 PrevRow = Row - 1;
	if (!RowRange.Contains(PrevRow))
	{
		return;
	}

	for (int32 Lane = MinLane; Lane <= MaxLane; ++Lane)
	{
		// ★壁の上には立てないので、出発点も「乗れる足場」で見る
		if (!HasLandableStep(PrevRow, Lane))
		{
			continue;
		}

		// ★行き先も同様。3つとも壁なら進めないので塞がっている扱い
		const bool bAnyForward =
			HasLandableStep(Row, Lane)
			|| HasLandableStep(Row, Lane - 1)
			|| HasLandableStep(Row, Lane + 1);

		if (bAnyForward)
		{
			continue;
		}

		// 行き場が無い。3つのうち1つを足場に変える。
		// 真正面を優先し、次に左右をランダムに選ぶ
		const int32 Choices[3] = { Lane, Lane - 1, Lane + 1 };
		const int32 Pick = (FMath::RandBool()) ? 1 : 2;
		const int32 Order[3] = { Choices[0], Choices[Pick], Choices[3 - Pick] };

		for (int32 i = 0; i < 3; ++i)
		{
			const int32 L = Order[i];
			if (L < MinLane - 1 || L > MaxLane + 1)
			{
				continue;
			}

			// そこが壁なら通常マスに直す。穴なら足場を作る
			if (AStairStep* S = GetStep(Row, L))
			{
				Holes.Remove(Key(Row, L));
				S->Setup(Row, L, EStairTile::Normal);
					ApplyStepTransform(S, Row, L, EStairTile::Normal);
			}
			else
			{
				Holes.Remove(Key(Row, L));
				SpawnStep(Row, L, EStairTile::Normal);
			}
			break;
		}
	}
}

// =====================================================================
// 生成
// =====================================================================

void UStairTerrain::GenerateRow(int32 Row, int32 MinLane, int32 MaxLane, float T)
{
	// タイトル背景など、穴を減らしたい場合は上書きできる
	const float HoleDensity = (HoleDensityOverride >= 0.f)
		? HoleDensityOverride
		: (Config ? Config->GetHoleDensity(T) : 0.2f);
	const float RedChance = Config ? Config->GetRedChance(T) : 0.03f;
	const int32 RedGap = Config ? Config->RedMinRowGap : 8;

	// 最初の行を全部足場にするかどうか（タイトル背景では土台に見えるので切る）
	const bool bFirstRow = (Row <= 0) && bSolidFirstRow;

	for (int32 Lane = MinLane; Lane <= MaxLane; ++Lane)
	{
		if (HasStep(Row, Lane) || IsKnownHole(Row, Lane))
		{
			continue; // 既に確定済み
		}

		bool bHole = false;

		if (!bFirstRow && FMath::FRand() < HoleDensity)
		{
			// 穴にしたい。制約を1マスずつ確認する
			Holes.Add(Key(Row, Lane));

			const bool bBadRun = WouldExceedHorizontalRun(Row, Lane);
			const bool bBadCluster = bBadRun ? false : WouldExceedCluster(Row, Lane);

			if (bBadRun || bBadCluster)
			{
				Holes.Remove(Key(Row, Lane)); // 穴にできない
			}
			else
			{
				bHole = true;
			}
		}

		if (!bHole)
		{
			EStairTile Tile = EStairTile::Normal;

			// ★壁にするか判定。出現率は穴の半分。
			//   壁も「通れないマス」なので、置いた結果として
			//   横の連続が上限を超えるなら置かない。
			const float WallChance = (WallChanceOverride >= 0.f)
				? WallChanceOverride
				: HoleDensity * (Config ? Config->WallChanceRatio : 0.5f);

			bool bWall = false;
			if (!bFirstRow && FMath::FRand() < WallChance)
			{
				// 仮に置いて制約を見る。壁は Steps 側で判定するので
				// 先に足場を作ってから確認し、駄目なら種別を戻す
				SpawnStep(Row, Lane, EStairTile::Wall);

				if (WouldExceedHorizontalRun(Row, Lane))
				{
					// この位置は塞げない。通常マスに戻す
					if (AStairStep* S = GetStep(Row, Lane))
					{
						S->Setup(Row, Lane, EStairTile::Normal);
						ApplyStepTransform(S, Row, Lane, EStairTile::Normal);
					}
				}
				else
				{
					bWall = true;
				}
			}

			if (!bWall)
			{
				// 赤マスにするか判定。最低間隔を守る
				if (!bFirstRow
					&& (Row - LastRedRow) >= RedGap
					&& FMath::FRand() < RedChance)
				{
					Tile = EStairTile::Red;
					LastRedRow = Row;
				}

				if (!HasStep(Row, Lane))
				{
					SpawnStep(Row, Lane, Tile);
				}
			}
		}
	}

	RowRange.Add(Row, TPair<int32, int32>(MinLane, MaxLane));

	// 直前の行から必ず進めるようにする
	EnsureReachability(Row, MinLane, MaxLane);
}

void UStairTerrain::UpdateAround(int32 PlayerRow, int32 PlayerLane, float T)
{
	if (!Config || !StepClass)
	{
		return;
	}

	const int32 Radius = Config->LaneRadius;
	const int32 Ahead = Config->GenerateAhead;
	// タイトル背景など、後ろを残したくない場合は上書きできる
	const int32 Behind = (KeepBehindOverride >= 0)
		? KeepBehindOverride : Config->KeepBehind;

	const int32 MinLane = PlayerLane - Radius;
	const int32 MaxLane = PlayerLane + Radius;

	// ---- 前方を生成する。行は必ず若い順に確定させる ----
	const int32 TargetRow = PlayerRow + Ahead;
	for (int32 Row = FMath::Max(0, GeneratedUpTo + 1); Row <= TargetRow; ++Row)
	{
		GenerateRow(Row, MinLane, MaxLane, T);
		GeneratedUpTo = Row;
	}

	// ---- 横に移動した場合、既存の行の左右を継ぎ足す ----
	// ここを忘れるとチャンク境界で穴だらけの帯ができる
	for (int32 Row = FMath::Max(0, PlayerRow - Behind); Row <= TargetRow; ++Row)
	{
		TPair<int32, int32>* Range = RowRange.Find(Row);
		if (!Range)
		{
			continue;
		}

		if (MinLane < Range->Key)
		{
			for (int32 Lane = MinLane; Lane < Range->Key; ++Lane)
			{
				if (!HasStep(Row, Lane) && !IsKnownHole(Row, Lane))
				{
					// 端は必ず足場にする。境界での不整合を避けるため
					SpawnStep(Row, Lane, EStairTile::Normal);
				}
			}
			Range->Key = MinLane;
		}
		if (MaxLane > Range->Value)
		{
			for (int32 Lane = Range->Value + 1; Lane <= MaxLane; ++Lane)
			{
				if (!HasStep(Row, Lane) && !IsKnownHole(Row, Lane))
				{
					SpawnStep(Row, Lane, EStairTile::Normal);
				}
			}
			Range->Value = MaxLane;
		}
	}

	// ---- 後方を片付ける ----
	const int32 CutRow = PlayerRow - Behind;
	if (CutRow > 0)
	{
		TArray<int64> ToRemove;
		for (const TPair<int64, TObjectPtr<AStairStep>>& P : Steps)
		{
			const int32 Row = (int32)(P.Key >> 20);
			if (Row < CutRow)
			{
				if (P.Value)
				{
					P.Value->Destroy();
				}
				ToRemove.Add(P.Key);
			}
		}
		for (int64 K : ToRemove)
		{
			Steps.Remove(K);
		}

		for (int32 Row = CutRow - 40; Row < CutRow; ++Row)
		{
			RowRange.Remove(Row);
		}
	}
}
