#include "SimplePixelShader.h"

#include "Classes/Engine/TextureRenderTarget2D.h"
#include "Classes/Engine/World.h"
#include "GlobalShader.h"
#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
#include "SceneUtils.h"
#include "SceneInterface.h"
#include "ShaderParameterUtils.h"
#include "Logging/MessageLog.h"
#include "Internationalization/Internationalization.h"

#define LOCTEXT_NAMESPACE "SimplePixelShader"  

struct FTextureVertex{
	FVector4 Position;
	FVector2D UV;
};

class FTextureVertexDeclaration : public FRenderResource
{
public:
	FVertexDeclarationRHIRef VertexDeclarationRHI;
	virtual void InitRHI() override
	{
		FVertexDeclarationElementList Elements;
		uint32 Stride = sizeof(FTextureVertex);
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FTextureVertex, Position), VET_Float2, 0, Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FTextureVertex, UV), VET_Float2, 1, Stride));
		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}
	virtual void ReleaseRHI() override
	{
		VertexDeclarationRHI.SafeRelease();
	}
};

class FSimplePixelShader : public FGlobalShader
{
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
        OutEnvironment.SetDefine(TEXT("TEST_MICRO"), 1);  
    }
    FSimplePixelShader(){}
    FSimplePixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		 SimpleColorVal.Bind(Initializer.ParameterMap, TEXT("SimpleColor")); 
		 TextureVal.Bind(Initializer.ParameterMap, TEXT("TextureVal"));
		 TextureSampler.Bind(Initializer.ParameterMap, TEXT("TextureSampler"));
	}
	template<typename TShaderRHIParamRef>
    void SetParameters(FRHICommandListImmediate& RHICmdList,const TShaderRHIParamRef ShaderRHI,	const FLinearColor &MyColor,const FTextureRHIParamRef& TextureRHI)
    {
        SetShaderValue(RHICmdList, ShaderRHI, SimpleColorVal, MyColor); 
		SetTextureParameter(RHICmdList, ShaderRHI, TextureVal, TextureSampler,TStaticSamplerState<SF_Trilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),TextureRHI);
    }

    virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << SimpleColorVal<< TextureVal<< TextureSampler;
		return bShaderHasOutdatedParameters;
	}
private:
    FShaderParameter SimpleColorVal;

	FShaderResourceParameter TextureVal;
	FShaderResourceParameter TextureSampler;
};

class FSimplePixelShaderVS : public FSimplePixelShader
{
    DECLARE_SHADER_TYPE(FSimplePixelShaderVS, Global);
public:
    FSimplePixelShaderVS(){}

  	FSimplePixelShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FSimplePixelShader(Initializer)
	{
	}
};

class FSimplePixelShaderPS : public FSimplePixelShader
{
    DECLARE_SHADER_TYPE(FSimplePixelShaderPS, Global);
public:
    FSimplePixelShaderPS(){}

  	FSimplePixelShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FSimplePixelShader(Initializer)
	{
	}
};

IMPLEMENT_SHADER_TYPE(, FSimplePixelShaderVS, TEXT("/Plugin/BRPlugins/Private/SimplePixelShader.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, FSimplePixelShaderPS, TEXT("/Plugin/BRPlugins/Private/SimplePixelShader.usf"), TEXT("MainPS"), SF_Pixel)

static void DrawTestShaderRenderTarget_RenderThread(  
    FRHICommandListImmediate& RHICmdList,   
    FTextureRenderTargetResource* OutputRenderTargetResource,  
    ERHIFeatureLevel::Type FeatureLevel,  
    const FName TextureRenderTargetName,  
    const FLinearColor MyColor,
	const FTextureRHIParamRef TextureRHI
)  
{  
    check(IsInRenderingThread());  
 
#if WANTS_DRAW_MESH_EVENTS  
    FString EventName;  
    TextureRenderTargetName.ToString(EventName);  
    SCOPED_DRAW_EVENTF(RHICmdList, SceneCapture, TEXT("ShaderTest %s"), *EventName);  
#else  
    SCOPED_DRAW_EVENT(RHICmdList, DrawTestShaderRenderTarget_RenderThread);  
#endif  
	FRHIRenderPassInfo RPInfo(OutputRenderTargetResource->GetRenderTargetTexture(), ERenderTargetActions::DontLoad_Store, OutputRenderTargetResource->TextureRHI);
	RHICmdList.BeginRenderPass(RPInfo, TEXT("DrawUVDisplacement"));
	
		FIntPoint DisplacementMapResolution(OutputRenderTargetResource->GetSizeX(), OutputRenderTargetResource->GetSizeY());

		// Update viewport.
		RHICmdList.SetViewport(
			0, 0, 0.f,
			DisplacementMapResolution.X, DisplacementMapResolution.Y, 1.f);

		// Get shaders.
		TShaderMap<FGlobalShaderType>* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
		TShaderMapRef< FSimplePixelShaderVS > VertexShader(GlobalShaderMap);
		TShaderMapRef< FSimplePixelShaderPS > PixelShader(GlobalShaderMap);

		FTextureVertexDeclaration VertexDeclaration;
		VertexDeclaration.InitRHI();

		// Set the graphic pipeline state.
		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;
		//GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetVertexDeclarationFVector4();
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = VertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

		// Update viewport.
		RHICmdList.SetViewport(
			0, 0, 0.f,
			OutputRenderTargetResource->GetSizeX(), OutputRenderTargetResource->GetSizeY(), 1.f);

		// Update shader uniform parameters.
		VertexShader->SetParameters(RHICmdList, VertexShader->GetVertexShader(),MyColor,TextureRHI);
		PixelShader->SetParameters(RHICmdList, PixelShader->GetPixelShader(),MyColor,TextureRHI);

		// Draw grid.
		/*
		理论上4.22应该用这个函数来绘制，但是不知道为什么有问题
		RHICmdList.DrawPrimitive(0, 2, 1);
		*/
		
		FTextureVertex Vertices[4];
		Vertices[0].Position.Set(-1.0f, 1.0f, 0, 1.0f);
		Vertices[1].Position.Set(1.0f, 1.0f, 0, 1.0f);
		Vertices[2].Position.Set(-1.0f, -1.0f, 0, 1.0f);
		Vertices[3].Position.Set(1.0f, -1.0f, 0, 1.0f);
		Vertices[0].UV.Set(0,1.0f);
		Vertices[1].UV.Set(1.0f,1.0f);
		Vertices[2].UV.Set(0,0);
		Vertices[3].UV.Set(1.0f,0.0);
		static const uint16 Indices[6] =
		{
			0, 1, 2,
			2, 1, 3
		};

		DrawIndexedPrimitiveUP(
			RHICmdList,
			PT_TriangleList,
			0,
			ARRAY_COUNT(Vertices),
			2,
			Indices,
			sizeof(Indices[0]),
			Vertices,
			sizeof(Vertices[0])
		);
		
		// Resolve render target. 
		
		RHICmdList.CopyToResolveTarget(
			OutputRenderTargetResource->GetRenderTargetTexture(),
			OutputRenderTargetResource->TextureRHI,
			FResolveParams());
		
		
	
	RHICmdList.EndRenderPass();
}  
 
USimplePixelShaderBlueprintLibrary::USimplePixelShaderBlueprintLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{ }

void USimplePixelShaderBlueprintLibrary::DrawTestShaderRenderTarget(const UObject* WorldContextObject, UTextureRenderTarget2D* OutputRenderTarget, FLinearColor MyColor,UTexture* MyTexture)
{  
    check(IsInGameThread());  
 
    if (!OutputRenderTarget)  
    {  
        return;  
    }  
 
    FTextureRenderTargetResource* TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();  
	FTextureRHIParamRef TextureRHI = MyTexture->TextureReference.TextureReferenceRHI;
	const UWorld* World = WorldContextObject->GetWorld();
    ERHIFeatureLevel::Type FeatureLevel = World->Scene->GetFeatureLevel();  
    FName TextureRenderTargetName = OutputRenderTarget->GetFName();  
    ENQUEUE_RENDER_COMMAND(CaptureCommand)(  
        [TextureRenderTargetResource, FeatureLevel, MyColor, TextureRenderTargetName,TextureRHI](FRHICommandListImmediate& RHICmdList)
        {  
            DrawTestShaderRenderTarget_RenderThread(RHICmdList,TextureRenderTargetResource, FeatureLevel, TextureRenderTargetName, MyColor,TextureRHI);  
        }  
    );  
}  

#undef LOCTEXT_NAMESPACE  