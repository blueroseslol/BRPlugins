// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimationBlueprintLibrary.h" 
#include "AnimBlueprintLibrary.generated.h"

/**
 * 
 */
UCLASS()
class BRPLUGINS_API UAnimBlueprintLibrary : public UAnimationBlueprintLibrary
{
	GENERATED_BODY()
public:
	/** Adds a Float Key to the specified Animation Curve inside of the given Animation Sequence */
	UFUNCTION(BlueprintCallable, Category = "AnimationBlueprintLibrary|Curves")
	static void AddFloatCurveKeyWithType(UAnimSequence* AnimationSequence, FName CurveName, const float Time, const float Value, EInterpCurveMode InterpMode);

	/** Adds a multiple of Float Keys to the specified Animation Curve inside of the given Animation Sequence */
	UFUNCTION(BlueprintCallable, Category = "AnimationBlueprintLibrary|Curves")
	static void AddFloatCurveKeysWithType(UAnimSequence* AnimationSequence, FName CurveName, const TArray<float>& Times, const TArray<float>& Values, EInterpCurveMode InterpMode);

	template <typename DataType, typename CurveClass> 
	static void AddCurveKeysInternal(UAnimSequence* AnimationSequence, FName CurveName, const TArray<float>& Times, const TArray<DataType>& KeyData, ERawCurveTrackTypes CurveType, EInterpCurveMode InterpMode);
};
