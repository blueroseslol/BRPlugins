// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "UMG/MultipleProgressBar.h"
#include "Slate/SlateBrushAsset.h"
#include "Math/UnrealMathUtility.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UMultipleProgressBar

static FProgressBarStyle* DefaultProgressBarStyle = nullptr;

UMultipleProgressBar::UMultipleProgressBar(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (DefaultProgressBarStyle == nullptr)
	{
		// HACK: THIS SHOULD NOT COME FROM CORESTYLE AND SHOULD INSTEAD BE DEFINED BY ENGINE TEXTURES/PROJECT SETTINGS
		DefaultProgressBarStyle = new FProgressBarStyle(FCoreStyle::Get().GetWidgetStyle<FProgressBarStyle>("ProgressBar"));

		// Unlink UMG default colors from the editor settings colors.
		DefaultProgressBarStyle->UnlinkColors();
	}

	WidgetStyle = *DefaultProgressBarStyle;
	WidgetStyle.FillImage.TintColor = FLinearColor::White;

	BarFillType = EMultipleProgressBarFillType::LeftToRight;
	BorderPadding = FVector2D(0, 0);

	PercentArray = TArray<float>();
	FillColorAndOpacityArray = TArray<FLinearColor>();
}

void UMultipleProgressBar::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyProgressBar.Reset();
}

TSharedRef<SWidget> UMultipleProgressBar::RebuildWidget()
{
	MyProgressBar = SNew(SMultipleProgressBar);

	return MyProgressBar.ToSharedRef();
}

void UMultipleProgressBar::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	TAttribute< TOptional<TArray<float> >> PercentBinding=OPTIONAL_BINDING_CONVERT(TArray<float>, PercentArray, TOptional<TArray<float>>, ConvertFloatToOptionalFloatArray);
	TAttribute<TArray<FSlateColor>> FillColorAndOpacityBinding = PROPERTY_BINDING(TArray<FSlateColor>, FillColorAndOpacityArray);

	MyProgressBar->SetStyle(&WidgetStyle);
	MyProgressBar->SetBarFillType(BarFillType);
	MyProgressBar->SetBorderPadding(BorderPadding);

	MyProgressBar->SetPercentArray(PercentBinding);
	MyProgressBar->SetFillColorAndOpacityArray(FillColorAndOpacityBinding);
}

void UMultipleProgressBar::SetFillColorAndOpacityArray(TArray<FLinearColor> InColorArray)
{
	FillColorAndOpacityArray = InColorArray;
	if (MyProgressBar.IsValid())
	{
		MyProgressBar->SetFillColorAndOpacityArray(InColorArray);
	}
}

void UMultipleProgressBar::SetPercentArray(TArray<float> InPercentArray)
{
	PercentArray = InPercentArray;
	if (MyProgressBar.IsValid())
	{
		MyProgressBar->SetPercentArray(InPercentArray);
	}
}

void UMultipleProgressBar::PostLoad()
{
	Super::PostLoad();

}

#if WITH_EDITOR

const FText UMultipleProgressBar::GetPaletteCategory()
{
	return LOCTEXT("Common", "Common");
}

void UMultipleProgressBar::OnCreationFromPalette()
{
	FillColorAndOpacityArray.Add(FLinearColor(0, 0.5f, 1.0f));
}

#endif

#undef LOCTEXT_NAMESPACE
