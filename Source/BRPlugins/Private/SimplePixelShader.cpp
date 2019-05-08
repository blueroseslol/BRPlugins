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
#include "Containers/DynamicRHIResourceArray.h"

#define LOCTEXT_NAMESPACE "SimplePixelShader"  

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FSimpleUniformStructParameters, )
SHADER_PARAMETER(FVector4, Color1)
SHADER_PARAMETER(FVector4, Color2)
SHADER_PARAMETER(FVector4, Color3)
SHADER_PARAMETER(FVector4, Color4)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FSimpleUniformStructParameters, "SimpleUniformStruct");

struct FTextureVertex{
	FVector4 Position;
	FVector2D UV;
};

class FRectangleVertexBuffer : public FVertexBuffer
{
public:
	/** Initialize the RHI for this rendering resource */
	void InitRHI() override 
	{
		TResourceArray<FTextureVertex, VERTEXBUFFER_ALIGNMENT> Vertices;
		Vertices.SetNumUninitialized(6);
	
		Vertices[0].Position = FVector4(1, 1, 0, 1);
		Vertices[0].UV = FVector2D(1, 1);

		Vertices[1].Position = FVector4(-1, 1, 0, 1);
		Vertices[1].UV = FVector2D(0, 1);

		Vertices[2].Position = FVector4(1, -1, 0, 1);
		Vertices[2].UV = FVector2D(1, 0);

		Vertices[3].Position = FVector4(-1, -1, 0, 1);
		Vertices[3].UV = FVector2D(0, 0);

		//The final two vertices are used for the triangle optimization (a single triangle spans the entire viewport )
		Vertices[4].Position = FVector4(-1, 1, 0, 1);
		Vertices[4].UV = FVector2D(-1, 1);

		Vertices[5].Position = FVector4(1, -1, 0, 1);
		Vertices[5].UV = FVector2D(1, -1);

		// Create vertex buffer. Fill buffer with initial data upon creation
		FRHIResourceCreateInfo CreateInfo(&Vertices);
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
};

class FRectangleIndexBuffer : public FIndexBuffer
{
public:
	/** Initialize the RHI for this rendering resource */
	void InitRHI() override 
	{
		// Indices 0 - 5 are used for rendering a quad. Indices 6 - 8 are used for triangle optimization.
		const uint16 Indices[] = { 0, 1, 2, 2, 1, 3, 0, 4, 5 };

		TResourceArray<uint16, INDEXBUFFER_ALIGNMENT> IndexBuffer;
		uint32 NumIndices = ARRAY_COUNT(Indices);
		IndexBuffer.AddUninitialized(NumIndices);
		FMemory::Memcpy(IndexBuffer.GetData(), Indices, NumIndices * sizeof(uint16));

		// Create index buffer. Fill buffer with initial data upon creation
		FRHIResourceCreateInfo CreateInfo(&IndexBuffer);
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint16), IndexBuffer.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
};

TGlobalResource<FRectangleVertexBuffer> GRectangleVertexBuffer;
TGlobalResource<FRectangleIndexBuffer> GRectangleIndexBuffer;

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
		FSimpleUniformStructParameters Parameters;
		Parameters.Color1 = FVector4(1.0f, 0.0f, 0.0f, 1.0f);
		Parameters.Color2 = FVector4(0.0f, 1.0f, 0.0f, 1.0f);
		Parameters.Color3 = FVector4(0.0f, 0.0f, 1.0f, 1.0f);
		Parameters.Color4 = FVector4(1.0f, 0.0f, 1.0f, 1.0f);

		SetUniformBufferParameterImmediate(RHICmdList, PixelShader->GetPixelShader(), PixelShader->GetUniformBufferParameter<FSimpleUniformStructParameters>(), Parameters);

		VertexShader->SetParameters(RHICmdList, VertexShader->GetVertexShader(),MyColor,TextureRHI);
		PixelShader->SetParameters(RHICmdList, PixelShader->GetPixelShader(),MyColor,TextureRHI);

		RHICmdList.SetStreamSource(0, GRectangleVertexBuffer.VertexBufferRHI, 0);

		RHICmdList.DrawIndexedPrimitive(
			GRectangleIndexBuffer.IndexBufferRHI,
			/*BaseVertexIndex=*/ 0,
			/*MinIndex=*/ 0,
			/*NumVertices=*/ 4,
			/*StartIndex=*/ 0,
			/*NumPrimitives=*/ 2,
			/*NumInstances=*/ 1);
		
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