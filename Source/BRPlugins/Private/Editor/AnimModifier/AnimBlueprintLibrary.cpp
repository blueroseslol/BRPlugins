// Fill out your copyright notice in the Description page of Project Settings.

#include "Editor/AnimModifier/AnimBlueprintLibrary.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimMetaData.h"
#include "Animation/AnimNotifies/AnimNotifyState.h" 
#include "Animation/Skeleton.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "BonePose.h" 

#include "AnimationRuntime.h"

DEFINE_LOG_CATEGORY_STATIC(LogAnimBlueprintLibrary, Verbose, All);

void UAnimBlueprintLibrary::AddFloatCurveKeyWithType(UAnimSequence* AnimationSequence, FName CurveName, const float Time, const float Value, EInterpCurveMode InterpMode)
{
	//UAnimationBlueprintLibrary::AddFloatCurveKey(AnimationSequence, CurveName, Time, Value);

	if (AnimationSequence)
	{
		TArray<float> TimeArray;
		TArray<float> ValueArray;

		TimeArray.Add(Time);
		ValueArray.Add(Value);

		AddCurveKeysInternal<float, FFloatCurve>(AnimationSequence, CurveName, TimeArray, ValueArray, ERawCurveTrackTypes::RCT_Float, InterpMode);
	}
	else
	{
		UE_LOG(LogAnimBlueprintLibrary, Warning, TEXT("Invalid Animation Sequence for AddFloatCurveKey"));
	}
}

void UAnimBlueprintLibrary::AddFloatCurveKeysWithType(UAnimSequence* AnimationSequence, FName CurveName, const TArray<float>& Times, const TArray<float>& Values, EInterpCurveMode InterpMode)
{
	if (AnimationSequence)
	{
		if (Times.Num() == Values.Num())
		{
			AddCurveKeysInternal<float, FFloatCurve>(AnimationSequence, CurveName, Times, Values, ERawCurveTrackTypes::RCT_Float, InterpMode);
		}
		else
		{
			UE_LOG(LogAnimBlueprintLibrary, Warning, TEXT("Number of Time values %i does not match the number of Values %i in AddFloatCurveKeys"), Times.Num(), Values.Num());
		}
	}
	else
	{
		UE_LOG(LogAnimBlueprintLibrary, Warning, TEXT("Invalid Animation Sequence for AddFloatCurveKeys"));
	}

}


template <typename DataType, typename CurveClass>
void UAnimBlueprintLibrary::AddCurveKeysInternal(UAnimSequence* AnimationSequence, FName CurveName, const TArray<float>& Times, const TArray<DataType>& KeyData, ERawCurveTrackTypes CurveType, EInterpCurveMode InterpMode)
{
	checkf(Times.Num() == KeyData.Num(), TEXT("Not enough key data supplied"));

	const FName ContainerName = RetrieveContainerNameForCurve(AnimationSequence, CurveName);

	if (ContainerName != NAME_None)
	{
		// Retrieve smart name for curve
		const FSmartName CurveSmartName = RetrieveSmartNameForCurve(AnimationSequence, CurveName, ContainerName);

		// Retrieve the curve by name
		CurveClass* Curve = static_cast<CurveClass*>(AnimationSequence->RawCurveData.GetCurveData(CurveSmartName.UID, CurveType));
		if (Curve)
		{
			const int32 NumKeys = KeyData.Num();
			for (int32 KeyIndex = 0; KeyIndex < NumKeys; ++KeyIndex)
			{
				//Curve->UpdateOrAddKey(, );

				FKeyHandle handle= Curve->FloatCurve.UpdateOrAddKey(Times[KeyIndex], KeyData[KeyIndex]);

				FRichCurveKey& InKey = Curve->FloatCurve.GetKey(handle);

				InKey.InterpMode = RCIM_Linear;
				InKey.TangentWeightMode = RCTWM_WeightedNone;
				InKey.TangentMode = RCTM_Auto;

				if (InterpMode == CIM_Constant)
				{
					InKey.InterpMode = RCIM_Constant;
				}
				else if (InterpMode == CIM_Linear)
				{
					InKey.InterpMode = RCIM_Linear;
				}
				else
				{
					InKey.InterpMode = RCIM_Cubic;

					if (InterpMode == CIM_CurveAuto || InterpMode == CIM_CurveAutoClamped)
					{
						InKey.TangentMode = RCTM_Auto;
					}
					else if (InterpMode == CIM_CurveBreak)
					{
						InKey.TangentMode = RCTM_Break;
					}
					else if (InterpMode == CIM_CurveUser)
					{
						InKey.TangentMode = RCTM_User;
					}
				}


			}

			AnimationSequence->BakeTrackCurvesToRawAnimation();
		}
	}
}