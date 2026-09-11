#include "StairTerrain.h"
#include "StairStep.h"
#include "StairConfig.h"
#include "Engine/World.h"
#include "Algo/Reverse.h"

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
	// ★譜面の道と横幅の制限は消さない。
	//   Initialize は地形を作り直すだけで、譜面が変わるわけではない。
}

void UStairTerrain::SetChartPath(const TMap<int64, EStairTile>& InPath,
	const TSet<int64>& InClear, const TSet<int32>& InRedRows,
	const TSet<int32>& InHoleRows)
{
	ChartPath = InPath;
	ChartClear = InClear;
	ChartRedRows = InRedRows;
	ChartHoleRows = InHoleRows;
	bChartRoad = true;
}

void UStairTerrain::ClearChartPath()
{
	ChartPath.Empty();
	ChartClear.Empty();
	ChartRedRows.Empty();
	ChartHoleRows.Empty();
	bChartRoad = false;
}

void UStairTerrain::ClearRedRow(int32 Row)
{
	ChartRedRows.Remove(Row);

	// 既に置いてある足場も普通の床に塗り直す
	if (const TPair<int32, int32>* Range = RowRange.Find(Row))
	{
		for (int32 Lane = Range->Key; Lane <= Range->Value; ++Lane)
		{
			if (AStairStep* S = GetStep(Row, Lane))
			{
				if (S->GetTile() == EStairTile::Red)
				{
					S->Setup(Row, Lane, EStairTile::Normal);
					ApplyStepTransform(S, Row, Lane, EStairTile::Normal);
				}
			}
		}
	}
}

void UStairTerrain::CollapseRow(int32 Row)
{
	// ---- その行を丸ごと壊す ----
	TArray<int64> Doomed;
	for (const TPair<int64, TObjectPtr<AStairStep>>& P : Steps)
	{
		if (RowOf(P.Key) == Row)
		{
			if (P.Value) { P.Value->Destroy(); }
			Doomed.Add(P.Key);
		}
	}
	for (int64 K : Doomed)
	{
		Steps.Remove(K);
	}

	// ---- それより上を1段ぶん下げる ----
	// ★若い行から順に動かす。移動先の行は直前に空いているので、
	//   同じ座標に2つ入ることがない。
	TArray<int64> Lifted;
	for (const TPair<int64, TObjectPtr<AStairStep>>& P : Steps)
	{
		if (RowOf(P.Key) > Row)
		{
			Lifted.Add(P.Key);
		}
	}
	Lifted.Sort();

	for (int64 K : Lifted)
	{
		TObjectPtr<AStairStep> S = Steps.FindRef(K);
		Steps.Remove(K);

		const int32 NewRow = RowOf(K) - 1;
		const int32 Lane = LaneOf(K);

		if (S)
		{
			// ★動かす前の「見た目の」位置を覚えておく。
			//   スライドの途中ならその途中の位置が入るので、
			//   何段まとめて詰めても動きが途切れない。
			const FVector Before = S->GetActorLocation();

			S->Row = NewRow;
			S->Lane = Lane;
			ApplyStepTransform(S, NewRow, Lane, S->GetTile());

			// 座標を書き換えただけだと瞬間移動に見えるので、滑らせる
			S->StartSlideFrom(Before,
				Config ? Config->RowShiftSlideSeconds : 0.12f);
		}
		Steps.Add(Key(NewRow, Lane), S);
	}

	// ---- 穴・確定範囲・譜面の道も同じように詰める ----
	auto ShiftKeySet = [Row](TSet<int64>& Set)
	{
		TSet<int64> Next;
		Next.Reserve(Set.Num());
		for (int64 K : Set)
		{
			const int32 R = RowOf(K);
			if (R == Row) { continue; }
			Next.Add((R > Row) ? Key(R - 1, LaneOf(K)) : K);
		}
		Set = MoveTemp(Next);
	};

	ShiftKeySet(Holes);
	ShiftKeySet(ChartClear);

	auto ShiftRowSet = [Row](TSet<int32>& Set)
	{
		TSet<int32> Next;
		for (int32 R : Set)
		{
			if (R == Row) { continue; }
			Next.Add((R > Row) ? (R - 1) : R);
		}
		Set = MoveTemp(Next);
	};

	ShiftRowSet(ChartRedRows);
	ShiftRowSet(ChartHoleRows);

	{
		TMap<int64, EStairTile> Next;
		Next.Reserve(ChartPath.Num());
		for (const TPair<int64, EStairTile>& P : ChartPath)
		{
			const int32 R = RowOf(P.Key);
			if (R == Row) { continue; }
			Next.Add((R > Row) ? Key(R - 1, LaneOf(P.Key)) : P.Key, P.Value);
		}
		ChartPath = MoveTemp(Next);
	}

	{
		TMap<int32, TPair<int32, int32>> Next;
		for (const TPair<int32, TPair<int32, int32>>& P : RowRange)
		{
			if (P.Key == Row) { continue; }
			Next.Add((P.Key > Row) ? (P.Key - 1) : P.Key, P.Value);
		}
		RowRange = MoveTemp(Next);
	}

	if (GeneratedUpTo >= Row) { --GeneratedUpTo; }
	if (LastRedRow > Row)     { --LastRedRow; }
}

void UStairTerrain::ShiftLanesAbove(int32 FromRow, int32 Delta)
{
	if (Delta == 0)
	{
		return;
	}

	// ---- 足場を動かす ----
	TArray<int64> Moving;
	for (const TPair<int64, TObjectPtr<AStairStep>>& P : Steps)
	{
		if (RowOf(P.Key) > FromRow)
		{
			Moving.Add(P.Key);
		}
	}

	// ★移動先が空いている順に処理する。
	//   右へ寄せるなら右端から、左へ寄せるなら左端から。
	//   逆順でやると、まだ動かしていない足場を上書きして消してしまう。
	Moving.Sort();
	if (Delta > 0)
	{
		Algo::Reverse(Moving);
	}

	const float Slide = Config ? Config->RowShiftSlideSeconds : 0.12f;

	for (int64 K : Moving)
	{
		TObjectPtr<AStairStep> S = Steps.FindRef(K);
		Steps.Remove(K);

		const int32 Row = RowOf(K);
		const int32 NewLane = LaneOf(K) + Delta;

		if (S)
		{
			const FVector Before = S->GetActorLocation();
			S->Lane = NewLane;
			ApplyStepTransform(S, Row, NewLane, S->GetTile());
			S->StartSlideFrom(Before, Slide);
		}
		Steps.Add(Key(Row, NewLane), S);
	}

	// ---- 穴・譜面の道も同じだけ動かす ----
	auto ShiftKeySet = [FromRow, Delta](TSet<int64>& Set)
	{
		TSet<int64> Next;
		Next.Reserve(Set.Num());
		for (int64 K : Set)
		{
			const int32 R = RowOf(K);
			Next.Add((R > FromRow) ? Key(R, LaneOf(K) + Delta) : K);
		}
		Set = MoveTemp(Next);
	};

	ShiftKeySet(Holes);
	ShiftKeySet(ChartClear);

	{
		TMap<int64, EStairTile> Next;
		Next.Reserve(ChartPath.Num());
		for (const TPair<int64, EStairTile>& P : ChartPath)
		{
			const int32 R = RowOf(P.Key);
			Next.Add((R > FromRow) ? Key(R, LaneOf(P.Key) + Delta) : P.Key, P.Value);
		}
		ChartPath = MoveTemp(Next);
	}

	// 確定済みのレーン範囲も、動かした行だけずらす
	for (TPair<int32, TPair<int32, int32>>& P : RowRange)
	{
		if (P.Key > FromRow)
		{
			P.Value.Key += Delta;
			P.Value.Value += Delta;
		}
	}

	// ★横幅の制限は狭めず広げる。
	//   詰めた結果、元の範囲の外へ道が出ることがある。
	LaneMin = FMath::Min(LaneMin, LaneMin + Delta);
	LaneMax = FMath::Max(LaneMax, LaneMax + Delta);
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

		// WallHeightScale = 1 で「1段上と同じ高さ」。
		// 大きくすると、そこからさらに上へ伸びる。
		const float Target = (Thick + Extra)
			* FMath::Max(0.1f, Config->WallHeightScale);

		Scale.Z = Target / 100.f;
		Loc.Z += (Target - Thick) * 0.5f;
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
	// ★スタート地点から数段は穴も壁も出さない。
	//   開始直後にいきなり避けさせられるのを防ぐ。
	const int32 Safe = Config ? Config->SafeStartRows : 3;
	const bool bFirstRow = (Row <= Safe) && bSolidFirstRow;

	// ★赤マスから跳び越していく区間は、着地点の手前まで丸ごと穴にする。
	//   赤が横一列すべてなので、その先も床が続いていると
	//   なぜ5段も跳ぶのかが伝わらない。谷にして跳ぶ理由を見せる。
	//   跳び越える前提の区間なので、到達性の保証もしない。
	if (ChartHoleRows.Contains(Row))
	{
		for (int32 Lane = MinLane; Lane <= MaxLane; ++Lane)
		{
			if (HasStep(Row, Lane))
			{
				RemoveStep(Row, Lane);
			}
			Holes.Add(Key(Row, Lane));
		}
		RowRange.Add(Row, TPair<int32, int32>(MinLane, MaxLane));
		return;
	}

	// ★譜面で赤マスに指定された行は、横一列すべてを赤で埋める。
	//   赤はここでしか出さない。ランダムに現れると、
	//   譜面が指示していない場所で5段跳びが起きてしまう。
	if (ChartRedRows.Contains(Row))
	{
		for (int32 Lane = MinLane; Lane <= MaxLane; ++Lane)
		{
			Holes.Remove(Key(Row, Lane));

			if (AStairStep* S = GetStep(Row, Lane))
			{
				S->Setup(Row, Lane, EStairTile::Red);
				ApplyStepTransform(S, Row, Lane, EStairTile::Red);
			}
			else
			{
				SpawnStep(Row, Lane, EStairTile::Red);
			}
		}
		RowRange.Add(Row, TPair<int32, int32>(MinLane, MaxLane));
		LastRedRow = Row;
		return;
	}

	for (int32 Lane = MinLane; Lane <= MaxLane; ++Lane)
	{
		if (HasStep(Row, Lane) || IsKnownHole(Row, Lane))
		{
			continue; // 既に確定済み
		}

		// ★譜面が通る道は、ランダムに関係なく必ず足場にする。
		//   これで「指定どおりに操作すればきれいに進める」が保証される。
		EStairTile ChartTile;
		if (GetChartTile(Row, Lane, ChartTile))
		{
			SpawnStep(Row, Lane, ChartTile);
			continue;
		}

		// ★譜面の打ち込み中は全部床にする。
		//   穴や壁があると、置きたい位置まで進めない。
		if (bAllFloor)
		{
			SpawnStep(Row, Lane, EStairTile::Normal);
			continue;
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

			// ★同じレーンで壁が縦に続かないようにする。
			//   2つ重なると、避けたあとすぐまた避けることになり、
			//   逃げ道を確保しても通れない場面が生まれる。
			const bool bWallAbove = IsWall(Row - 1, Lane) || IsWall(Row + 1, Lane);

			// ★譜面がまっすぐ跳び越していく途中のマスには壁を置かない。
			//   穴なら上を通れるが、壁は背が高いので引っかかってしまう。
			const bool bMustBeClear = ChartClear.Contains(Key(Row, Lane));

			bool bWall = false;
			if (!bFirstRow && !bWallAbove && !bMustBeClear
				&& FMath::FRand() < WallChance)
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
				// 赤マスにするか判定。最低間隔を守る。
				// ★譜面どおりの道を敷いているときは出さない。
				//   赤は譜面が指定した行だけに置く。
				if (!bChartRoad
					&& !bFirstRow
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

	// ★横は無限に広げない。譜面が使う幅の外は作っても見えないだけ
	const int32 MinLane = FMath::Max(PlayerLane - Radius, LaneMin);
	const int32 MaxLane = FMath::Min(PlayerLane + Radius, LaneMax);

	if (MaxLane < MinLane)
	{
		return;   // 完全に範囲の外。これ以上作るものが無い
	}

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

		// ★跳び越す谷は端まで穴のまま。継ぎ足して床を作ってはいけない
		if (ChartHoleRows.Contains(Row))
		{
			for (int32 Lane = MinLane; Lane <= MaxLane; ++Lane)
			{
				Holes.Add(Key(Row, Lane));
			}
			Range->Key = FMath::Min(Range->Key, MinLane);
			Range->Value = FMath::Max(Range->Value, MaxLane);
			continue;
		}

		// ★赤の行に継ぎ足すときも赤にする。一列だけ色が途切れないように
		const EStairTile Fill = ChartRedRows.Contains(Row)
			? EStairTile::Red : EStairTile::Normal;

		if (MinLane < Range->Key)
		{
			for (int32 Lane = MinLane; Lane < Range->Key; ++Lane)
			{
				if (!HasStep(Row, Lane) && !IsKnownHole(Row, Lane))
				{
					// 端は必ず足場にする。境界での不整合を避けるため
					SpawnStep(Row, Lane, Fill);
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
					SpawnStep(Row, Lane, Fill);
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
