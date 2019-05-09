#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SimplePixelShader.generated.h"

USTRUCT(BlueprintType, meta = (ScriptName = "SimplePixelShaderTest"))
struct FSimpleUniformStruct 
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (WorldContext = "WorldContextObject"))
	FLinearColor Color1;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (WorldContext = "WorldContextObject"))
	FLinearColor Color2;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (WorldContext = "WorldContextObject"))
	FLinearColor Color3;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (WorldContext = "WorldContextObject"))
	FLinearColor Color4;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (WorldContext = "WorldContextObject"))
	int32 ColorIndex;
};

UCLASS(MinimalAPI, meta=(ScriptName="SimplePixelShaderTest"))
class USimplePixelShaderBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable, Category = "SimplePixelShader", meta = (WorldContext = "WorldContextObject"))
	static void DrawTestShaderRenderTarget(const UObject* WorldContextObject, UTextureRenderTarget2D* OutputRenderTarget,FLinearColor MyColor, UTexture* MyTexture,FSimpleUniformStruct UniformStruct);

	/*
		向UTexture2D写入数据
	*/
	UFUNCTION(BlueprintCallable, Category = "SimplePixelShader", meta = (WorldContext = "WorldContextObject"))
	static void TextureWriting(UTexture2D* TextureToBeWrite);


	UFUNCTION(BlueprintCallable, Category = "SimplePixelShader", meta = (WorldContext = "WorldContextObject"))
	static void CreateTexture(const FString& TextureName, const FString& PackageName);
};
