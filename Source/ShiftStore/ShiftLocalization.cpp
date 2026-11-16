#include "ShiftLocalization.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	TMap<FString, FString> Catalogues[ShiftLoc::LanguageCount];
	int32 ActiveLanguage = 0;
	bool bInitialized = false;
	const TCHAR* Codes[] = { TEXT("en"), TEXT("ja"), TEXT("zh-Hant"), TEXT("zh-Hans") };
	const TCHAR* Names[] = { TEXT("English"), TEXT("日本語"), TEXT("繁體中文"), TEXT("简体中文") };
}

void ShiftLoc::Initialize()
{
	if (bInitialized) { return; }
	bInitialized = true;
	for (int32 Index = 0; Index < LanguageCount; ++Index)
	{
		FString Json;
		TSharedPtr<FJsonObject> Root;
		const FString Path = FPaths::ProjectContentDir() / TEXT("Localization/Strings") / (LanguageCode(Index) + TEXT(".json"));
		if (FFileHelper::LoadFileToString(Json, *Path) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) && Root.IsValid())
		{
			for (const auto& Pair : Root->Values)
			{
				FString Value;
				if (Pair.Value->TryGetString(Value)) { Catalogues[Index].Add(FString(*Pair.Key), Value); }
			}
		}
		else { UE_LOG(LogTemp, Error, TEXT("Cannot load language pack: %s"), *Path); }
	}
	FString SavedCode;
	GConfig->GetString(TEXT("ShiftStore.Language"), TEXT("Code"), SavedCode, GGameUserSettingsIni);
	for (int32 Index = 0; Index < LanguageCount; ++Index)
	{
		if (SavedCode == Codes[Index]) { ActiveLanguage = Index; break; }
	}
	FString Error;
	if (!Validate(Error)) { UE_LOG(LogTemp, Error, TEXT("Language packs invalid: %s"), *Error); }
}

int32 ShiftLoc::CurrentLanguage() { return ActiveLanguage; }
FString ShiftLoc::LanguageCode(int32 Index) { return Codes[FMath::Clamp(Index, 0, LanguageCount - 1)]; }
FString ShiftLoc::NativeName(int32 Index) { return Names[FMath::Clamp(Index, 0, LanguageCount - 1)]; }

FString ShiftLoc::SettingsFile()
{
	const FConfigBranch* Branch = GConfig->FindBranch(FName(TEXT("GameUserSettings")), GGameUserSettingsIni);
	return Branch ? FPaths::ConvertRelativePathToFull(Branch->IniPath) : FString();
}

bool ShiftLoc::SetLanguage(int32 Index, bool bPersist)
{
	Initialize();
	if (Index < 0 || Index >= LanguageCount || Catalogues[Index].IsEmpty()) { return false; }
	ActiveLanguage = Index;
	if (bPersist)
	{
		GConfig->SetString(TEXT("ShiftStore.Language"), TEXT("Code"), Codes[Index], GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
	return true;
}

FString ShiftLoc::Text(const FString& Source)
{
	if (const FString* Value = Catalogues[ActiveLanguage].Find(Source)) { return *Value; }
	if (const FString* Value = Catalogues[0].Find(Source)) { return *Value; }
	return Source;
}

FString ShiftLoc::Format(const FString& Source, const TArray<FString>& Arguments)
{
	TArray<FStringFormatArg> LocalizedArguments;
	for (const FString& Argument : Arguments) { LocalizedArguments.Emplace(Text(Argument)); }
	return FString::Format(*Text(Source), LocalizedArguments);
}

bool ShiftLoc::Validate(FString& Error)
{
	if (Catalogues[0].IsEmpty()) { Error = TEXT("English catalogue is empty"); return false; }
	for (int32 Index = 1; Index < LanguageCount; ++Index)
	{
		if (Catalogues[Index].Num() != Catalogues[0].Num()) { Error = LanguageCode(Index) + TEXT(": key count mismatch"); return false; }
		for (const auto& Pair : Catalogues[0])
		{
			const FString* Value = Catalogues[Index].Find(Pair.Key);
			if (!Value || Value->IsEmpty()) { Error = LanguageCode(Index) + TEXT(": missing ") + Pair.Key; return false; }
		}
	}
	return true;
}
