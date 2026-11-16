#pragma once

#include "CoreMinimal.h"

// English source strings are stable catalogue keys; SKU names and save data stay language-independent.
namespace ShiftLoc
{
	constexpr int32 LanguageCount = 4;
	void Initialize();
	int32 CurrentLanguage();
	FString LanguageCode(int32 Index);
	FString NativeName(int32 Index);
	FString SettingsFile();
	bool SetLanguage(int32 Index, bool bPersist = true);
	FString Text(const FString& Source);
	FString Format(const FString& Source, const TArray<FString>& Arguments);
	bool Validate(FString& Error);
}
