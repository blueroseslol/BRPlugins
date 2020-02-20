// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "UMG/SMultipleProgressBar.h"
#include "Rendering/DrawElements.h"


void SMultipleProgressBar::Construct( const FArguments& InArgs )
{
	check(InArgs._Style);

	Style = InArgs._Style;
	BarFillType = InArgs._BarFillType;
	
	BackgroundImage = InArgs._BackgroundImage;
	FillImage = InArgs._FillImage;
	
	BorderPadding = InArgs._BorderPadding;
	
	SetCanTick(false);

	SetPercentArray(InArgs._PercentArray);
	FillColorAndOpacityArray = InArgs._FillColorAndOpacityArray;
}

void SMultipleProgressBar::SetPercentArray(TAttribute<TOptional<TArray<float> >> InPercentArray)
{
	if (!PercentArray.IdenticalTo(InPercentArray))
	{
		PercentArray = InPercentArray;

		Invalidate(EInvalidateWidget::LayoutAndVolatility);
	}
}

void SMultipleProgressBar::SetStyle(const FProgressBarStyle* InStyle)
{
	Style = InStyle;

	if (Style == nullptr)
	{
		FArguments Defaults;
		Style = Defaults._Style;
	}

	check(Style);

	Invalidate(EInvalidateWidget::Layout);
}

void SMultipleProgressBar::SetBarFillType(EMultipleProgressBarFillType::Type InBarFillType)
{
	if(BarFillType != InBarFillType)
	{
		BarFillType = InBarFillType;
		Invalidate(EInvalidateWidget::Paint);
	}
}

void SMultipleProgressBar::SetFillColorAndOpacityArray(TAttribute<TArray<FSlateColor>> InFillColorAndOpacityArray)
{
	if (!FillColorAndOpacityArray.IdenticalTo(InFillColorAndOpacityArray))
	{
		FillColorAndOpacityArray = InFillColorAndOpacityArray;
		Invalidate(EInvalidateWidget::Paint);
	}
}

void SMultipleProgressBar::SetBorderPadding(TAttribute< FVector2D > InBorderPadding)
{
	if(!BorderPadding.IdenticalTo(InBorderPadding))
	{
		BorderPadding = InBorderPadding;
		Invalidate(EInvalidateWidget::Layout);
	}
}

void SMultipleProgressBar::SetBackgroundImage(const FSlateBrush* InBackgroundImage)
{
	if(BackgroundImage != InBackgroundImage)
	{
		BackgroundImage = InBackgroundImage;
		Invalidate(EInvalidateWidget::Layout);
	}
}

void SMultipleProgressBar::SetFillImage(const FSlateBrush* InFillImage)
{
	if(FillImage != InFillImage)
	{
		FillImage = InFillImage;
		Invalidate(EInvalidateWidget::Layout);
	}
}

const FSlateBrush* SMultipleProgressBar::GetBackgroundImage() const
{
	return BackgroundImage ? BackgroundImage : &Style->BackgroundImage;
}

const FSlateBrush* SMultipleProgressBar::GetFillImage() const
{
	return FillImage ? FillImage : &Style->FillImage;
}

// Returns false if the clipping zone area is zero in which case we should skip drawing
bool PushTransformedClip(FSlateWindowElementList& OutDrawElements, const FGeometry& AllottedGeometry, FVector2D InsetPadding, FVector2D ProgressOrigin, FSlateRect Progress)
{
	const FSlateRenderTransform& Transform = AllottedGeometry.GetAccumulatedRenderTransform();

	const FVector2D MaxSize = AllottedGeometry.GetLocalSize() - ( InsetPadding * 2.0f );

	const FSlateClippingZone ClippingZone(Transform.TransformPoint(InsetPadding + (ProgressOrigin - FVector2D(Progress.Left, Progress.Top)) * MaxSize),
		Transform.TransformPoint(InsetPadding + FVector2D(ProgressOrigin.X + Progress.Right, ProgressOrigin.Y - Progress.Top) * MaxSize),
		Transform.TransformPoint(InsetPadding + FVector2D(ProgressOrigin.X - Progress.Left, ProgressOrigin.Y + Progress.Bottom) * MaxSize),
		Transform.TransformPoint(InsetPadding + (ProgressOrigin + FVector2D(Progress.Right, Progress.Bottom)) * MaxSize));

	if (ClippingZone.HasZeroArea())
	{
		return false;
	}

	OutDrawElements.PushClip(ClippingZone);
	return true;
}

int32 SMultipleProgressBar::OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	// Used to track the layer ID we will return.
	int32 RetLayerId = LayerId;

	bool bEnabled = ShouldBeEnabled( bParentEnabled );
	const ESlateDrawEffect DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	
	const FSlateBrush* CurrentFillImage = GetFillImage();
	const FLinearColor ColorAndOpacitySRGB = InWidgetStyle.GetColorAndOpacityTint();

	FVector2D BorderPaddingRef = BorderPadding.Get();

	const FSlateBrush* CurrentBackgroundImage = GetBackgroundImage();

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		RetLayerId++,
		AllottedGeometry.ToPaintGeometry(),
		CurrentBackgroundImage,
		DrawEffects,
		InWidgetStyle.GetColorAndOpacityTint() * CurrentBackgroundImage->GetTint( InWidgetStyle )
	);	
	

	EMultipleProgressBarFillType::Type ComputedBarFillType = BarFillType;
	if (GSlateFlowDirection == EFlowDirection::RightToLeft)
	{
		switch (ComputedBarFillType)
		{
		case EMultipleProgressBarFillType::LeftToRight:
			ComputedBarFillType = EMultipleProgressBarFillType::RightToLeft;
			break;
		case EMultipleProgressBarFillType::RightToLeft:
			ComputedBarFillType = EMultipleProgressBarFillType::LeftToRight;
			break;
		}
	}

	TArray<float> PercentArrayList = PercentArray.Get().GetValue();
	TArray<FSlateColor> FillColorAndOpacityArrayList = FillColorAndOpacityArray.Get();

	while (FillColorAndOpacityArrayList.Num() < PercentArrayList.Num())
	{
		FillColorAndOpacityArrayList.Add(FSlateColor());
	}

	for (int i = 0; i < PercentArrayList.Num(); i++)
	{
		const float ClampedFraction = FMath::Clamp(PercentArrayList[i], 0.0f, 1.0f);
		const FLinearColor FillColorAndOpacitySRGB = (InWidgetStyle.GetColorAndOpacityTint() * FillColorAndOpacityArrayList[i].GetColor(InWidgetStyle) * CurrentFillImage->GetTint(InWidgetStyle));
		
		switch (ComputedBarFillType)
		{
			case EMultipleProgressBarFillType::RightToLeft:
			{
				if (PushTransformedClip(OutDrawElements, AllottedGeometry, BorderPaddingRef, FVector2D(1, 0), FSlateRect(ClampedFraction, 0, 0, 1)))
				{
					// Draw Fill
					FSlateDrawElement::MakeBox(
						OutDrawElements,
						RetLayerId++,
						AllottedGeometry.ToPaintGeometry(
							FVector2D::ZeroVector,
							FVector2D(AllottedGeometry.GetLocalSize().X, AllottedGeometry.GetLocalSize().Y)),
						CurrentFillImage,
						DrawEffects,
						FillColorAndOpacitySRGB
					);

					OutDrawElements.PopClip();
				}
				break;
			}
			case EMultipleProgressBarFillType::FillFromCenter:
			{
				float HalfFrac = ClampedFraction / 2.0f;
				if (PushTransformedClip(OutDrawElements, AllottedGeometry, BorderPaddingRef, FVector2D(0.5, 0.5), FSlateRect(HalfFrac, HalfFrac, HalfFrac, HalfFrac)))
				{
					// Draw Fill
					FSlateDrawElement::MakeBox(
						OutDrawElements,
						RetLayerId++,
						AllottedGeometry.ToPaintGeometry(
							FVector2D((AllottedGeometry.GetLocalSize().X * 0.5f) - ((AllottedGeometry.GetLocalSize().X * (ClampedFraction))*0.5), 0.0f),
							FVector2D(AllottedGeometry.GetLocalSize().X * (ClampedFraction), AllottedGeometry.GetLocalSize().Y)),
						CurrentFillImage,
						DrawEffects,
						FillColorAndOpacitySRGB
					);

					OutDrawElements.PopClip();
				}
				break;
			}
			case EMultipleProgressBarFillType::TopToBottom:
			{
				if (PushTransformedClip(OutDrawElements, AllottedGeometry, BorderPaddingRef, FVector2D(0, 0), FSlateRect(0, 0, 1, ClampedFraction)))
				{
					// Draw Fill
					FSlateDrawElement::MakeBox(
						OutDrawElements,
						RetLayerId++,
						AllottedGeometry.ToPaintGeometry(
							FVector2D::ZeroVector,
							FVector2D(AllottedGeometry.GetLocalSize().X, AllottedGeometry.GetLocalSize().Y)),
						CurrentFillImage,
						DrawEffects,
						FillColorAndOpacitySRGB
					);

					OutDrawElements.PopClip();
				}
				break;
			}
			case EMultipleProgressBarFillType::BottomToTop:
			{
				if (PushTransformedClip(OutDrawElements, AllottedGeometry, BorderPaddingRef, FVector2D(0, 1), FSlateRect(0, ClampedFraction, 1, 0)))
				{
					FSlateRect ClippedAllotedGeometry = FSlateRect(AllottedGeometry.AbsolutePosition, AllottedGeometry.AbsolutePosition + AllottedGeometry.GetLocalSize() * AllottedGeometry.Scale);
					ClippedAllotedGeometry.Top = ClippedAllotedGeometry.Bottom - ClippedAllotedGeometry.GetSize().Y * ClampedFraction;

					// Draw Fill
					FSlateDrawElement::MakeBox(
						OutDrawElements,
						RetLayerId++,
						AllottedGeometry.ToPaintGeometry(
							FVector2D::ZeroVector,
							FVector2D(AllottedGeometry.GetLocalSize().X, AllottedGeometry.GetLocalSize().Y)),
						CurrentFillImage,
						DrawEffects,
						FillColorAndOpacitySRGB
					);

					OutDrawElements.PopClip();
				}
				break;
			}
			case EMultipleProgressBarFillType::LeftToRight:
			default:
			{
				if (PushTransformedClip(OutDrawElements, AllottedGeometry, BorderPaddingRef, FVector2D(0, 0), FSlateRect(0, 0, ClampedFraction, 1)))
				{
					// Draw Fill
					FSlateDrawElement::MakeBox(
						OutDrawElements,
						RetLayerId++,
						AllottedGeometry.ToPaintGeometry(
							FVector2D::ZeroVector,
							FVector2D(AllottedGeometry.GetLocalSize().X, AllottedGeometry.GetLocalSize().Y)),
						CurrentFillImage,
						DrawEffects,
						FillColorAndOpacitySRGB
					);

					OutDrawElements.PopClip();
				}
				break;
			}
		}
	}
	
	return RetLayerId - 1;
}

FVector2D SMultipleProgressBar::ComputeDesiredSize( float ) const
{
	return Style->MarqueeImage.ImageSize;
}

bool SMultipleProgressBar::ComputeVolatility() const
{
	return SLeafWidget::ComputeVolatility() || PercentArray.IsBound() || FillColorAndOpacityArray.IsBound() || BorderPadding.IsBound();
}