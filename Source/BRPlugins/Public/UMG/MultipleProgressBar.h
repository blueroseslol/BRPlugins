// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SWidget.h"
#include "SMultipleProgressBar.h"
#include "Components/Widget.h"
#include "MultipleProgressBar.generated.h"

class USlateBrushAsset;

DECLARE_DYNAMIC_DELEGATE_RetVal(TArray<float>, FGetFloatArray);
DECLARE_DYNAMIC_DELEGATE_RetVal(TArray<FLinearColor>, FGetSlateColorArray);

/**
 * The progress bar widget is a simple bar that fills up that can be restyled to fit any number of uses.
 *
 * * No Children
 */
UCLASS()
class BRPLUGINS_API UMultipleProgressBar : public UWidget
{
	GENERATED_UCLASS_BODY()
	
public:

	/** The progress bar style */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style", meta=( DisplayName="Style" ))
	FProgressBarStyle WidgetStyle;

	/** Percent Array */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Progress)
	TArray<float> PercentArray;

	/** Defines if this progress bar fills Left to right or right to left */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Progress)
	TEnumAsByte<EMultipleProgressBarFillType::Type> BarFillType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Progress)
	FVector2D BorderPadding;

	/** A bindable delegate to allow logic to drive the text of the widget */
	UPROPERTY()
	FGetFloatArray PercentArrayDelegate;

	/** Fill Color and Opacity */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Appearance)
	TArray<FLinearColor> FillColorAndOpacityArray;

	/** */
	UPROPERTY()
	FGetSlateColorArray FillColorAndOpacityArrayDelegate;
public:

	/** Sets the current value of the ProgressBar. */
	UFUNCTION(BlueprintCallable, Category = "Progress")
	void SetPercentArray(TArray<float> InPercentArray);

	/** Sets the fill color of the progress bar. */
	UFUNCTION(BlueprintCallable, Category = "Progress")
	void SetFillColorAndOpacityArray(TArray<FLinearColor> InColorArray);

public:
	
	//~ Begin UWidget Interface
	virtual void SynchronizeProperties() override;
	//~ End UWidget Interface

	//~ Begin UVisual Interface
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	//~ End UVisual Interface

	//~ Begin UObject Interface
	virtual void PostLoad() override;
	//~ End UObject Interface

#if WITH_EDITOR
	//~ Begin UWidget Interface
	virtual const FText GetPaletteCategory() override;
	virtual void OnCreationFromPalette() override;
	//~ End UWidget Interface
#endif

protected:
	/** Native Slate Widget */
	TSharedPtr<SMultipleProgressBar> MyProgressBar;

	//~ Begin UWidget Interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	//~ End UWidget Interface

	TOptional<TArray<float>> ConvertFloatToOptionalFloatArray(TAttribute<TArray<float>> InFloatArray) const
	{
		return InFloatArray.Get();
	}

	PROPERTY_BINDING_IMPLEMENTATION(TArray<FSlateColor>, FillColorAndOpacityArray);
};
