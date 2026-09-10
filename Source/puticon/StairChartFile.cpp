#include "StairChartFile.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	// EStairNote の宣言順と同じ。Forward / Left / Right / Red
	const TCHAR* NoteCode(EStairNote Type)
	{
		switch (Type)
		{
		case EStairNote::Left:  return TEXT("L");
		case EStairNote::Right: return TEXT("R");
		case EStairNote::Red:   return TEXT("X");
		default:                return TEXT("F");
		}
	}

	bool CodeToNote(TCHAR C, EStairNote& Out)
	{
		switch (FChar::ToUpper(C))
		{
		case TEXT('F'): Out = EStairNote::Forward; return true;
		case TEXT('L'): Out = EStairNote::Left;    return true;
		case TEXT('R'): Out = EStairNote::Right;   return true;
		case TEXT('X'): Out = EStairNote::Red;     return true;
		default: return false;
		}
	}
}

FString StairChartFile::PathFor(int32 SongIndex)
{
	return FPaths::ProjectSavedDir() / TEXT("Charts")
		/ FString::Printf(TEXT("Chart_%d.txt"), SongIndex);
}

bool StairChartFile::Load(int32 SongIndex, TArray<FStairChartNote>& OutNotes)
{
	const FString Path = PathFor(SongIndex);

	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		return false;
	}

	// "Notes:" の次の行が本体
	const FString Marker = TEXT("Notes:");
	const int32 At = Text.Find(Marker, ESearchCase::IgnoreCase, ESearchDir::FromStart);
	if (At == INDEX_NONE)
	{
		return false;
	}

	FString Body = Text.Mid(At + Marker.Len());
	Body.TrimStartAndEndInline();

	// 本体は1行。念のため2行目以降は捨てる
	int32 NL = INDEX_NONE;
	if (Body.FindChar(TEXT('\n'), NL))
	{
		Body = Body.Left(NL);
	}
	Body.ReplaceInline(TEXT("\r"), TEXT(""));

	TArray<FString> Tokens;
	Body.ParseIntoArray(Tokens, TEXT(","), true);

	// ★同じ位置に2つあれば後ろを残す。跳ぶ向きは1回に1つのため
	TMap<int32, EStairNote> Uniq;
	for (FString Token : Tokens)
	{
		Token.TrimStartAndEndInline();
		if (Token.IsEmpty()) { continue; }

		FString Left, Right;
		if (!Token.Split(TEXT(":"), &Left, &Right)) { continue; }

		Left.TrimStartAndEndInline();
		Right.TrimStartAndEndInline();
		if (Left.IsEmpty() || Right.IsEmpty() || !Left.IsNumeric()) { continue; }

		EStairNote Type;
		if (!CodeToNote(Right[0], Type)) { continue; }

		Uniq.Add(FCString::Atoi(*Left), Type);
	}

	OutNotes.Reset();
	for (const TPair<int32, EStairNote>& P : Uniq)
	{
		FStairChartNote N;
		N.Slot = P.Key;
		N.Type = P.Value;
		OutNotes.Add(N);
	}
	OutNotes.Sort();

	return true;
}

bool StairChartFile::Save(int32 SongIndex, const TArray<FStairChartNote>& Notes,
	const FString& Title, float BPM, int32 Subdivision)
{
	FString Line;
	for (int32 i = 0; i < Notes.Num(); ++i)
	{
		Line += FString::Printf(TEXT("%d:%s"), Notes[i].Slot, NoteCode(Notes[i].Type));
		if (i < Notes.Num() - 1) { Line += TEXT(","); }
	}

	const FString Text = FString::Printf(
		TEXT("曲: %s\nBPM: %.2f\n分割: 1拍を %d 分割\n個数: %d\n")
		TEXT("種類: F=正面 L=左 R=右 X=赤マス\nNotes:\n%s\n"),
		*Title, BPM, Subdivision, Notes.Num(), *Line);

	return FFileHelper::SaveStringToFile(Text, *PathFor(SongIndex));
}
