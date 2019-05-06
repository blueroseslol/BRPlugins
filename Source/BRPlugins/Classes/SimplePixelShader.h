#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SimplePixelShader.generated.h"


UCLASS(MinimalAPI, meta=(ScriptName="SimplePixelShaderTest"))
class USimplePixelShaderBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable, Category = "SimplePixelShader", meta = (WorldContext = "WorldContextObject"))
	static void DrawTestShaderRenderTarget(const UObject* WorldContextObject, UTextureRenderTarget2D* OutputRenderTarget,FLinearColor MyColor, UTexture* MyTexture);
};