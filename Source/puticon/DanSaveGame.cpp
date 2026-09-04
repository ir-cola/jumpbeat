#include "DanSaveGame.h"
#include "Kismet/GameplayStatics.h"

UDanSaveGame* UDanSaveGame::LoadOrCreate()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName(), 0))
	{
		if (UDanSaveGame* Loaded =
			Cast<UDanSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName(), 0)))
		{
			return Loaded;
		}
	}

	return Cast<UDanSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UDanSaveGame::StaticClass()));
}

bool UDanSaveGame::SubmitScore(int32 Score)
{
	UDanSaveGame* Save = LoadOrCreate();
	if (!Save)
	{
		return false;
	}

	FDanRankEntry Entry;
	Entry.Score = Score;
	Entry.Date  = FDateTime::Now();

	Save->TopScores.Add(Entry);

	// 降順に並べて上位3件だけ残す
	Save->TopScores.Sort([](const FDanRankEntry& A, const FDanRankEntry& B)
	{
		return A.Score > B.Score;
	});

	const bool bRanked = Save->TopScores.IndexOfByPredicate(
		[Score](const FDanRankEntry& E) { return E.Score == Score; }) < MaxEntries;

	if (Save->TopScores.Num() > MaxEntries)
	{
		Save->TopScores.SetNum(MaxEntries);
	}

	UGameplayStatics::SaveGameToSlot(Save, SlotName(), 0);
	return bRanked;
}
