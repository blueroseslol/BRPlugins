#include "Rendering/SimpleRenderingExample.h"
#include "Engine/TextureRenderTarget2D.h"

#include "PipelineStateCache.h"
#include "GlobalShader.h"
#include "ShaderCompilerCore.h"

namespace SimpleRenderingExample
{
	/*
	 * Shader 
	 */
	class FSimpleGlobalShader : public FGlobalShader
	{
		DECLARE_INLINE_TYPE_LAYOUT(FSimpleGlobalShader, NonVirtual);
	public:
		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
		{
			return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
		}

		static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
		{
			FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
			OutEnvironment.SetDefine(TEXT("TEST_MICRO"), 1);  
		}

		FSimpleGlobalShader(){}

		FSimpleGlobalShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):FGlobalShader(Initializer)
		{
			 SimpleColorVal.Bind(Initializer.ParameterMap, TEXT("SimpleColor")); 
			 TextureVal.Bind(Initializer.ParameterMap, TEXT("TextureVal"));
			 TextureSampler.Bind(Initializer.ParameterMap, TEXT("TextureSampler"));
		}

		template<typename TRHIShader>
		void SetParameters(FRHICommandList& RHICmdList,TRHIShader* ShaderRHI,const FLinearColor &MyColor, FTexture2DRHIRef InInputTexture)
		{
			SetShaderValue(RHICmdList, ShaderRHI, SimpleColorVal, MyColor); 
			SetTextureParameter(RHICmdList, ShaderRHI, TextureVal, TextureSampler,TStaticSamplerState<SF_Trilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(), InInputTexture);
		}

	private:
		LAYOUT_FIELD(FShaderResourceParameter, TextureVal);
		LAYOUT_FIELD(FShaderResourceParameter, TextureSampler);
		LAYOUT_FIELD(FShaderParameter, SimpleColorVal);
	};

	class FSimpleVertexShader : public FSimpleGlobalShader
	{
		DECLARE_GLOBAL_SHADER(FSimpleVertexShader)
	public:
		FSimpleVertexShader(){}

  		FSimpleVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):FSimpleGlobalShader(Initializer){}
	};

	class FSimplePixelShader : public FSimpleGlobalShader
	{
		DECLARE_GLOBAL_SHADER(FSimplePixelShader)
	public:
		FSimplePixelShader(){}

  		FSimplePixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):FSimpleGlobalShader(Initializer){}
	};

	class FSimpleComputeShader : public FGlobalShader
	{
		DECLARE_GLOBAL_SHADER(FSimpleComputeShader)

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
		{
			return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
		}

		static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
		{
			FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
			OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
		}

	public:
		FSimpleComputeShader() {}
		FSimpleComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
			: FGlobalShader(Initializer)
		{
			OutTexture.Bind(Initializer.ParameterMap, TEXT("OutTexture"));
		}

		void SetParameters(FRHICommandList& RHICmdList, FRHIUnorderedAccessView* InOutUAV, FSimpleShaderParameter UniformStruct)
		{
			FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();
			if (OutTexture.IsBound())
				RHICmdList.SetUAVParameter(ComputeShaderRHI, OutTexture.GetBaseIndex(), InOutUAV);

			FSimpleUniformStructParameters ShaderStructData;
			ShaderStructData.Color1 = UniformStruct.Color1;
			ShaderStructData.Color2 = UniformStruct.Color2;
			ShaderStructData.Color3 = UniformStruct.Color3;
			ShaderStructData.Color4 = UniformStruct.Color4;
			ShaderStructData.ColorIndex = UniformStruct.ColorIndex;

			SetUniformBufferParameterImmediate(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FSimpleUniformStructParameters>(), ShaderStructData);
		}

		void UnbindBuffers(FRHICommandList& RHICmdList)
		{
			FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();
			if (OutTexture.IsBound())
				RHICmdList.SetUAVParameter(ComputeShaderRHI, OutTexture.GetBaseIndex(), nullptr);
		}
	private:
		LAYOUT_FIELD(FShaderResourceParameter, OutTexture);	
	};

	IMPLEMENT_SHADER_TYPE(, FSimpleComputeShader, TEXT("/BRPlugins/Private/SimpleComputeShader.usf"), TEXT("MainCS"), SF_Compute)
	IMPLEMENT_SHADER_TYPE(, FSimpleVertexShader, TEXT("/BRPlugins/Private/SimplePixelShader.usf"), TEXT("MainVS"), SF_Vertex)
	IMPLEMENT_SHADER_TYPE(, FSimplePixelShader, TEXT("/BRPlugins/Private/SimplePixelShader.usf"), TEXT("MainPS"), SF_Pixel)

	/*
	 * Tradition Method
	 */
    void GlobalShaderCompute(FRHICommandListImmediate &RHIImmCmdList, FTexture2DRHIRef InTexRenderTargetRHIture, FSimpleShaderParameter InParameter)
    {
		check(IsInRenderingThread());

		const ERHIFeatureLevel::Type FeatureLevel = GMaxRHIFeatureLevel;
		TShaderMapRef<FSimpleComputeShader> ComputeShader(GetGlobalShaderMap(FeatureLevel));
		RHIImmCmdList.SetComputeShader(RHIImmCmdList.GetBoundComputeShader());
		
		FIntPoint Size = InTexRenderTargetRHIture->GetSizeXY();

		FRHIResourceCreateInfo CreateInfo;

		FTexture2DRHIRef Texture = RHICreateTexture2D(Size.X, Size.Y, PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		FUnorderedAccessViewRHIRef TextureUAV = RHICreateUnorderedAccessView(Texture);
		ComputeShader->SetParameters(RHIImmCmdList, TextureUAV, InParameter);
		DispatchComputeShader(RHIImmCmdList, ComputeShader, Size.X / 32, Size.Y / 32, 1);
		ComputeShader->UnbindBuffers(RHIImmCmdList);

		GlobalShaderDraw(RHIImmCmdList, InTexRenderTargetRHIture, InParameter,FLinearColor(), Texture);
    }

    void GlobalShaderDraw(FRHICommandListImmediate &RHIImmCmdList, FTexture2DRHIRef RenderTargetRHI, FSimpleShaderParameter InParameter,const FLinearColor InColor, FTexture2DRHIRef InTexture)
    {
        check(IsInRenderingThread());

	#if WANTS_DRAW_MESH_EVENTS  
		SCOPED_DRAW_EVENTF(RHIImmCmdList, SceneCapture, TEXT("SimplePixelShaderPassTest"));
	#else  
		SCOPED_DRAW_EVENT(RHIImmCmdList, GlobalShaderDraw);
	#endif  
		RHIImmCmdList.TransitionResource(ERHIAccess::EWritable, RenderTargetRHI);

		FRHIRenderPassInfo RPInfo(RenderTargetRHI, ERenderTargetActions::DontLoad_Store, RenderTargetRHI);
		RHIImmCmdList.BeginRenderPass(RPInfo, TEXT("SimplePixelShaderPass"));

		// Get shaders.
		const ERHIFeatureLevel::Type FeatureLevel = GMaxRHIFeatureLevel; 
		FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
		TShaderMapRef<FSimpleVertexShader> VertexShader(GlobalShaderMap);
		TShaderMapRef<FSimplePixelShader> PixelShader(GlobalShaderMap);

		FTextureVertexDeclaration VertexDeclaration;
		VertexDeclaration.InitRHI();

		// Set the graphic pipeline state.
		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHIImmCmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = VertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
		SetGraphicsPipelineState(RHIImmCmdList, GraphicsPSOInit);

		// Update viewport.
		RHIImmCmdList.SetViewport(
			0, 0, 0.f,RenderTargetRHI->GetSizeX(), RenderTargetRHI->GetSizeY(), 1.f);

		// Update shader uniform parameters.
		FSimpleUniformStructParameters Parameters;
		Parameters.Color1 = InParameter.Color1;
		Parameters.Color2 = InParameter.Color2;
		Parameters.Color3 = InParameter.Color3;
		Parameters.Color4 = InParameter.Color4;
		Parameters.ColorIndex = InParameter.ColorIndex;

		SetUniformBufferParameterImmediate(RHIImmCmdList, PixelShader.GetPixelShader(), PixelShader->GetUniformBufferParameter<FSimpleUniformStructParameters>(), Parameters);
		VertexShader->SetParameters(RHIImmCmdList, VertexShader.GetVertexShader(), InColor, InTexture);
		PixelShader->SetParameters(RHIImmCmdList, PixelShader.GetPixelShader(), InColor, InTexture);

		RHIImmCmdList.SetStreamSource(0, GRectangleVertexBuffer.VertexBufferRHI, 0);
		RHIImmCmdList.DrawIndexedPrimitive(
			GRectangleIndexBuffer.IndexBufferRHI,
			/*BaseVertexIndex=*/ 0,
			/*MinIndex=*/ 0,
			/*NumVertices=*/ 4,
			/*StartIndex=*/ 0,
			/*NumPrimitives=*/ 2,
			/*NumInstances=*/ 1);

		RHIImmCmdList.EndRenderPass();
    }
} // namespace SimpleRenderingExample

void USimpleRenderingExampleBlueprintLibrary::UseGlobalShaderCompute(const UObject *WorldContextObject, UTextureRenderTarget2D *OutputRenderTarget, FSimpleShaderParameter Parameter)
{
	check(IsInGameThread());
	FTexture2DRHIRef RenderTargetRHI = OutputRenderTarget->GameThread_GetRenderTargetResource()->GetRenderTargetTexture();

	ENQUEUE_RENDER_COMMAND(CaptureCommand)(
		[RenderTargetRHI, Parameter](FRHICommandListImmediate& RHICmdList) {
			SimpleRenderingExample::GlobalShaderCompute(RHICmdList, RenderTargetRHI, Parameter);
		});
}

void USimpleRenderingExampleBlueprintLibrary::UseGlobalShaderDraw(const UObject *WorldContextObject, UTextureRenderTarget2D *OutputRenderTarget, FSimpleShaderParameter Parameter, FLinearColor InColor, UTexture2D *InTexture)
{
	check(IsInGameThread());
	FTexture2DRHIRef RenderTargetRHI= OutputRenderTarget->GameThread_GetRenderTargetResource()->GetRenderTargetTexture();
	FTexture2DRHIRef InTextureRHI = InTexture->Resource->TextureRHI->GetTexture2D();

	ENQUEUE_RENDER_COMMAND(CaptureCommand)(
		[RenderTargetRHI,Parameter,InColor,InTextureRHI](FRHICommandListImmediate& RHICmdList) {
			SimpleRenderingExample::GlobalShaderDraw(RHICmdList, RenderTargetRHI, Parameter, InColor, InTextureRHI);
		});
}