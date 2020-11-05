#include "Rendering/SimpleRenderingExample.h"
#include "Engine/TextureRenderTarget2D.h"

#include "PipelineStateCache.h"

#include "GlobalShader.h"
#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"
#include "RHIStaticStates.h"
#include "ShaderParameterUtils.h"
#include "PixelShaderUtils.h"

namespace SimpleRenderingExample
{
	/*
	 * Vertex Resource
	 */
	TGlobalResource<FTextureVertexDeclaration> GTextureVertexDeclaration;
	TGlobalResource<FRectangleVertexBuffer> GRectangleVertexBuffer;
	TGlobalResource<FRectangleIndexBuffer> GRectangleIndexBuffer;

	/*
	 * Shader 
	 */
	class FSimpleRDGComputeShader : public FGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FSimpleRDGComputeShader);
		SHADER_USE_PARAMETER_STRUCT(FSimpleRDGComputeShader, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FSimpleUniformStructParameters, SimpleUniformStruct)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutTexture)
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters)
		{
			return RHISupportsComputeShaders(Parameters.Platform);
		}
	};

	class FSimpleRDGGlobalShader : public FGlobalShader
	{
	public:
		SHADER_USE_PARAMETER_STRUCT(FSimpleRDGGlobalShader, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FSimpleUniformStructParameters, SimpleUniformStruct)
		SHADER_PARAMETER_TEXTURE(Texture2D, TextureVal)
		SHADER_PARAMETER_SAMPLER(SamplerState, TextureSampler)
		SHADER_PARAMETER(FVector4, SimpleColor)
		RENDER_TARGET_BINDING_SLOTS()
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters)
		{
			return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
		}
	};

	class FSimpleRDGVertexShader : public FSimpleRDGGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FSimpleRDGVertexShader);

		FSimpleRDGVertexShader() {}

		FSimpleRDGVertexShader(const ShaderMetaType::CompiledShaderInitializerType &Initializer) : FSimpleRDGGlobalShader(Initializer) {}
	};

	class FSimpleRDGPixelShader : public FSimpleRDGGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FSimpleRDGPixelShader);

		FSimpleRDGPixelShader() {}

		FSimpleRDGPixelShader(const ShaderMetaType::CompiledShaderInitializerType &Initializer) : FSimpleRDGGlobalShader(Initializer) {}
	};

	IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FSimpleUniformStructParameters, "SimpleUniformStruct");

	IMPLEMENT_GLOBAL_SHADER(FSimpleRDGComputeShader, "/BRPlugins/Private/SimpleComputeShader.usf", "MainCS", SF_Compute);
	IMPLEMENT_GLOBAL_SHADER(FSimpleRDGVertexShader, "/BRPlugins/Private/SimplePixelShader.usf", "MainVS", SF_Vertex);
	IMPLEMENT_GLOBAL_SHADER(FSimpleRDGPixelShader, "/BRPlugins/Private/SimplePixelShader.usf", "MainPS", SF_Pixel);

	/*
	 * Render Function 
	 */
	void RDGCompute(FRHICommandListImmediate &RHIImmCmdList, FTexture2DRHIRef RenderTargetRHI, FSimpleShaderParameter InParameter)
	{
		check(IsInRenderingThread());

		//Create PooledRenderTarget
		FPooledRenderTargetDesc RenderTargetDesc = FPooledRenderTargetDesc::Create2DDesc(RenderTargetRHI->GetSizeXY(),RenderTargetRHI->GetFormat(), FClearValueBinding::Black, TexCreate_None, TexCreate_RenderTargetable | TexCreate_ShaderResource | TexCreate_UAV, false);
		TRefCountPtr<IPooledRenderTarget> PooledRenderTarget;

		//RDG Begin
		FRDGBuilder GraphBuilder(RHIImmCmdList);
		FRDGTextureRef RDGRenderTarget = GraphBuilder.CreateTexture(RenderTargetDesc, TEXT("RDGRenderTarget"));

		//Setup Parameters
		FSimpleUniformStructParameters StructParameters;
		StructParameters.Color1 = InParameter.Color1;
		StructParameters.Color2 = InParameter.Color2;
		StructParameters.Color3 = InParameter.Color3;
		StructParameters.Color4 = InParameter.Color4;
		StructParameters.ColorIndex = InParameter.ColorIndex;

		FSimpleRDGComputeShader::FParameters *Parameters = GraphBuilder.AllocParameters<FSimpleRDGComputeShader::FParameters>();
		FRDGTextureUAVDesc UAVDesc(RDGRenderTarget);
		Parameters->SimpleUniformStruct = TUniformBufferRef<FSimpleUniformStructParameters>::CreateUniformBufferImmediate(StructParameters, UniformBuffer_SingleFrame);
		Parameters->OutTexture = GraphBuilder.CreateUAV(UAVDesc);

		//Get ComputeShader From GlobalShaderMap
		const ERHIFeatureLevel::Type FeatureLevel = GMaxRHIFeatureLevel; //ERHIFeatureLevel::SM5
		FGlobalShaderMap *GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
		TShaderMapRef<FSimpleRDGComputeShader> ComputeShader(GlobalShaderMap);

		//Compute Thread Group Count
		FIntVector ThreadGroupCount(
			RenderTargetRHI->GetSizeX() / 32,
			RenderTargetRHI->GetSizeY() / 32,
			1);

		//ValidateShaderParameters(PixelShader, Parameters);
		//ClearUnusedGraphResources(PixelShader, Parameters);

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("RDGCompute"),
			Parameters,
			ERDGPassFlags::Compute,
			[Parameters, ComputeShader, ThreadGroupCount](FRHICommandList &RHICmdList) {
				FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *Parameters, ThreadGroupCount);
			});

		GraphBuilder.QueueTextureExtraction(RDGRenderTarget, &PooledRenderTarget);
		GraphBuilder.Execute();

		//Copy Result To RenderTarget Asset
		RHIImmCmdList.CopyTexture(PooledRenderTarget->GetRenderTargetItem().ShaderResourceTexture, RenderTargetRHI->GetTexture2D(), FRHICopyTextureInfo());
		//RHIImmCmdList.CopyToResolveTarget(PooledRenderTarget->GetRenderTargetItem().ShaderResourceTexture, RenderTargetRHI->GetTexture2D(), FResolveParams());
	}

	void RDGDraw(FRHICommandListImmediate &RHIImmCmdList, FTexture2DRHIRef RenderTargetRHI, FSimpleShaderParameter InParameter, const FLinearColor InColor, FTexture2DRHIRef InTexture)
	{
		check(IsInRenderingThread());

		//Create PooledRenderTarget
		FPooledRenderTargetDesc RenderTargetDesc = FPooledRenderTargetDesc::Create2DDesc(RenderTargetRHI->GetSizeXY(),RenderTargetRHI->GetFormat(), FClearValueBinding::Black, TexCreate_None, TexCreate_RenderTargetable | TexCreate_ShaderResource | TexCreate_UAV, false);
		TRefCountPtr<IPooledRenderTarget> PooledRenderTarget;

		//RDG Begin
		FRDGBuilder GraphBuilder(RHIImmCmdList);
		FRDGTextureRef RDGRenderTarget = GraphBuilder.CreateTexture(RenderTargetDesc, TEXT("RDGRenderTarget"));

		//Setup Parameters
		FSimpleUniformStructParameters StructParameters;
		StructParameters.Color1 = InParameter.Color1;
		StructParameters.Color2 = InParameter.Color2;
		StructParameters.Color3 = InParameter.Color3;
		StructParameters.Color4 = InParameter.Color4;
		StructParameters.ColorIndex = InParameter.ColorIndex;

		FSimpleRDGPixelShader::FParameters *Parameters = GraphBuilder.AllocParameters<FSimpleRDGPixelShader::FParameters>();
		Parameters->TextureVal = InTexture;
		Parameters->TextureSampler = TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		Parameters->SimpleColor = InColor;
		Parameters->SimpleUniformStruct = TUniformBufferRef<FSimpleUniformStructParameters>::CreateUniformBufferImmediate(StructParameters, UniformBuffer_SingleFrame);
		Parameters->RenderTargets[0] = FRenderTargetBinding(RDGRenderTarget, ERenderTargetLoadAction::ENoAction);

		const ERHIFeatureLevel::Type FeatureLevel = GMaxRHIFeatureLevel; //ERHIFeatureLevel::SM5
		FGlobalShaderMap *GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
		TShaderMapRef<FSimpleRDGVertexShader> VertexShader(GlobalShaderMap);
		TShaderMapRef<FSimpleRDGPixelShader> PixelShader(GlobalShaderMap);

		//ValidateShaderParameters(PixelShader, Parameters);
		//ClearUnusedGraphResources(PixelShader, Parameters);

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("RDGDraw"),
			Parameters,
			ERDGPassFlags::Raster,
			[Parameters, VertexShader, PixelShader, GlobalShaderMap](FRHICommandList &RHICmdList) {
				FRHITexture2D *RT = Parameters->RenderTargets[0].GetTexture()->GetRHI()->GetTexture2D();
				RHICmdList.SetViewport(0, 0, 0.0f, RT->GetSizeX(), RT->GetSizeY(), 1.0f);

				FGraphicsPipelineStateInitializer GraphicsPSOInit;
				RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
				GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
				GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
				GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
				GraphicsPSOInit.PrimitiveType = PT_TriangleList;
				GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GTextureVertexDeclaration.VertexDeclarationRHI;

				GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
				GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
				SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
				RHICmdList.SetStencilRef(0);
				SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *Parameters);

				RHICmdList.SetStreamSource(0, GRectangleVertexBuffer.VertexBufferRHI, 0);
				RHICmdList.DrawIndexedPrimitive(
					GRectangleIndexBuffer.IndexBufferRHI,
					/*BaseVertexIndex=*/0,
					/*MinIndex=*/0,
					/*NumVertices=*/4,
					/*StartIndex=*/0,
					/*NumPrimitives=*/2,
					/*NumInstances=*/1);
			});

		GraphBuilder.QueueTextureExtraction(RDGRenderTarget, &PooledRenderTarget);
		GraphBuilder.Execute();

		//Copy Result To RenderTarget Asset
		RHIImmCmdList.CopyTexture(PooledRenderTarget->GetRenderTargetItem().ShaderResourceTexture, RenderTargetRHI->GetTexture2D(), FRHICopyTextureInfo());
	}
} // namespace SimpleRenderingExample

void USimpleRenderingExampleBlueprintLibrary::UseRDGComput(const UObject *WorldContextObject, UTextureRenderTarget2D *OutputRenderTarget, FSimpleShaderParameter Parameter)
{
	check(IsInGameThread());

	FTexture2DRHIRef RenderTargetRHI = OutputRenderTarget->GameThread_GetRenderTargetResource()->GetRenderTargetTexture();

	ENQUEUE_RENDER_COMMAND(CaptureCommand)
	(
		[RenderTargetRHI, Parameter](FRHICommandListImmediate &RHICmdList) {
			SimpleRenderingExample::RDGCompute(RHICmdList, RenderTargetRHI, Parameter);
		});
}

void USimpleRenderingExampleBlueprintLibrary::UseRDGDraw(const UObject *WorldContextObject, UTextureRenderTarget2D *OutputRenderTarget, FSimpleShaderParameter Parameter, FLinearColor InColor, UTexture2D *InTexture)
{
	check(IsInGameThread());

	FTexture2DRHIRef RenderTargetRHI = OutputRenderTarget->GameThread_GetRenderTargetResource()->GetRenderTargetTexture();
	FTexture2DRHIRef InTextureRHI = InTexture->Resource->TextureRHI->GetTexture2D();

	ENQUEUE_RENDER_COMMAND(CaptureCommand)
	(
		[RenderTargetRHI, Parameter, InColor, InTextureRHI](FRHICommandListImmediate &RHICmdList) {
			SimpleRenderingExample::RDGDraw(RHICmdList, RenderTargetRHI, Parameter, InColor, InTextureRHI);
		});
}