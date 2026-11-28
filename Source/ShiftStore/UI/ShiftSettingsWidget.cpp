#include "ShiftSettingsWidget.h"

#include "../ShiftLocalization.h"
#include "../ShiftPlayerController.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"

namespace
{
	bool IsSettingsLabelKey(const FString& Key)
	{
		static const TCHAR* Keys[] =
		{
			TEXT("Mouse Sensitivity"),
			TEXT("Lower is slower. Changes are saved automatically."),
			TEXT("Restore Default"),
			TEXT("Back")
		};

		for (const TCHAR* Candidate : Keys)
		{
			if (Key == Candidate)
			{
				return true;
			}
		}
		return false;
	}
}

void UShiftSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	LocalizedLabels.Reset();
	if (WidgetTree)
	{
		WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			if (UTextBlock* Block = Cast<UTextBlock>(Widget))
			{
				const FString Key = Block->GetText().ToString();
				if (IsSettingsLabelKey(Key))
				{
					LocalizedLabels.Emplace(Key, Block);
				}
			}
		});
	}
	RefreshLabels();

	AShiftPlayerController* PC = Cast<AShiftPlayerController>(GetOwningPlayer());
	if (!PC)
	{
		return;
	}

	if (SensitivitySlider)
	{
		SensitivitySlider->SetMinValue(AShiftPlayerController::MinSensitivity);
		SensitivitySlider->SetMaxValue(AShiftPlayerController::MaxSensitivity);
		SensitivitySlider->SetStepSize(0.05f);
		SensitivitySlider->SetValue(PC->MouseSensitivity());
		SensitivitySlider->OnValueChanged.AddDynamic(this, &UShiftSettingsWidget::OnSensitivityChanged);
	}
	if (RestoreButton)
	{
		RestoreButton->OnClicked.AddDynamic(this, &UShiftSettingsWidget::OnRestore);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddDynamic(this, &UShiftSettingsWidget::OnBack);
	}

	UpdateValueText(PC->MouseSensitivity());
}

void UShiftSettingsWidget::FocusSlider()
{
	if (SensitivitySlider)
	{
		SensitivitySlider->SetKeyboardFocus();
	}
}

void UShiftSettingsWidget::RefreshLabels()
{
	for (const TPair<FString, TWeakObjectPtr<UTextBlock>>& Pair : LocalizedLabels)
	{
		if (UTextBlock* Block = Pair.Value.Get())
		{
			Block->SetText(FText::FromString(ShiftLoc::Text(Pair.Key)));
		}
	}
}

void UShiftSettingsWidget::UpdateValueText(float Value)
{
	if (!SensitivityValue)
	{
		return;
	}
	const FString Text = ShiftLoc::Format(TEXT("Mouse sensitivity: %sx"),
		{ FString::Printf(TEXT("%.2f"), Value) });
	SensitivityValue->SetText(FText::FromString(Text));
}

void UShiftSettingsWidget::OnSensitivityChanged(float Value)
{
	if (AShiftPlayerController* PC = Cast<AShiftPlayerController>(GetOwningPlayer()))
	{
		PC->SetMouseSensitivity(Value, false);
	}
	UpdateValueText(Value);
}

void UShiftSettingsWidget::OnRestore()
{
	if (AShiftPlayerController* PC = Cast<AShiftPlayerController>(GetOwningPlayer()))
	{
		PC->SetMouseSensitivity(1.f, false);
	}
	if (SensitivitySlider)
	{
		SensitivitySlider->SetValue(1.f);
	}
	UpdateValueText(1.f);
}

void UShiftSettingsWidget::OnBack()
{
	if (AShiftPlayerController* PC = Cast<AShiftPlayerController>(GetOwningPlayer()))
	{
		const float Value = SensitivitySlider ? SensitivitySlider->GetValue() : 1.f;
		PC->SetMouseSensitivity(Value, true);
	}
	SetVisibility(ESlateVisibility::Collapsed);
	OnClosed.ExecuteIfBound();
}
