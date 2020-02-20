// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Attribute.h"
#include "Styling/SlateColor.h"
#include "Styling/SlateWidgetStyleAsset.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SLeafWidget.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
#include "SMultipleProgressBar.generated.h"

class FActiveTimerHandle;
class FPaintArgs;
class FSlateWindowElementList;

/**
 * SProgressBar Fill Type 
 */
UENUM(BlueprintType)
namespace EMultipleProgressBarFillType
{
	enum Type
	{
		// will fill up from the left side to the right
		LeftToRight,
		// will fill up from the right side to the left side
		RightToLeft,
		// will fill up from the center to the outer edges
		FillFromCenter,
		// will fill up from the top to the the bottom
		TopToBottom,
		// will fill up from the bottom to the the top
		BottomToTop,
	};
}


/** A progress bar widget.*/
class BRPLUGINS_API SMultipleProgressBar : public SLeafWidget
{

public:
	SLATE_BEGIN_ARGS(SMultipleProgressBar)
		: _Style( &FCoreStyle::Get().GetWidgetStyle<FProgressBarStyle>("ProgressBar") )
		, _BarFillType(EMultipleProgressBarFillType::LeftToRight)
		, _PercentArray(TOptional<TArray<float> > ())
		, _FillColorAndOpacityArray(TArray<FSlateColor >())
		, _BorderPadding( FVector2D(1,0) )
		, _BackgroundImage(nullptr)
		, _FillImage(nullptr)
		, _RefreshRate(2.0f)
		{
			_Visibility = EVisibility::SelfHitTestInvisible;
		}

		/** Style used for the progress bar */
		SLATE_STYLE_ARGUMENT( FProgressBarStyle, Style )

		/** Defines if this progress bar fills Left to right or right to left*/
		SLATE_ARGUMENT( EMultipleProgressBarFillType::Type, BarFillType )

		/** Used to determine the fill position of the progress bar ranging 0..1 */
		SLATE_ATTRIBUTE(TOptional<TArray<float> >, PercentArray)

		/** Fill Color and Opacity */
		SLATE_ATTRIBUTE( TArray<FSlateColor>, FillColorAndOpacityArray )

		/** Border Padding around fill bar */
		SLATE_ATTRIBUTE( FVector2D, BorderPadding )
	
		/** The brush to use as the background of the progress bar */
		SLATE_ARGUMENT(const FSlateBrush*, BackgroundImage)
	
		/** The brush to use as the fill image */
		SLATE_ARGUMENT(const FSlateBrush*, FillImage)

		/** Rate at which this widget is ticked when sleeping in seconds */
		SLATE_ARGUMENT(float, RefreshRate)

	SLATE_END_ARGS()

	/**
	 * Construct the widget
	 * 
	 * @param InArgs   A declaration from which to construct the widget
	 */
	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;
	virtual bool ComputeVolatility() const override;

	/** See attribute Percent */
	void SetPercentArray(TAttribute<TOptional <TArray<float> >> InPercentArray);
	
	/** See attribute Style */
	void SetStyle(const FProgressBarStyle* InStyle);
	
	/** See attribute BarFillType */
	void SetBarFillType(EMultipleProgressBarFillType::Type InBarFillType);
	
	/** See attribute SetFillColorAndOpacity */
	void SetFillColorAndOpacityArray(TAttribute<TArray<FSlateColor>> InFillColorAndOpacityArray);
	
	/** See attribute BorderPadding */
	void SetBorderPadding(TAttribute< FVector2D > InBorderPadding);
	
	/** See attribute BackgroundImage */
	void SetBackgroundImage(const FSlateBrush* InBackgroundImage);
	
	/** See attribute FillImage */
	void SetFillImage(const FSlateBrush* InFillImage);
	
private:

	/** Gets the current background image. */
	const FSlateBrush* GetBackgroundImage() const;
	/** Gets the current fill image */
	const FSlateBrush* GetFillImage() const;

private:
	
	/** The style of the progress bar */
	const FProgressBarStyle* Style;

	/** The text displayed over the progress bar */
	TAttribute< TOptional<TArray<float> >> PercentArray;

	EMultipleProgressBarFillType::Type BarFillType;

	/** Background image to use for the progress bar */
	const FSlateBrush* BackgroundImage;

	/** Foreground image to use for the progress bar */
	const FSlateBrush* FillImage;

	/** Fill Color and Opacity */
	TAttribute<TArray<FSlateColor> > FillColorAndOpacityArray;

	/** Border Padding */
	TAttribute<FVector2D> BorderPadding;
};

