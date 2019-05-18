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
#include "AssetRegistryModule.h"


#define LOCTEXT_NAMESPACE "SimplePixelShader"  

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FSimpleUniformStructParameters, )
SHADER_PARAMETER(FVector4, Color1)
SHADER_PARAMETER(FVector4, Color2)
SHADER_PARAMETER(FVector4, Color3)
SHADER_PARAMETER(FVector4, Color4)
SHADER_PARAMETER(uint32, ColorIndex)
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


class FSimpleComputeShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSimpleComputeShader, Global);

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

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << OutTexture;
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIParamRef InOutUAV, FSimpleUniformStruct UniformStruct)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		//		SetShaderValue(RHICmdList, ComputeShaderRHI, TargetHeight, InTargetHeight);
		//		SetTextureParameter(RHICmdList, ComputeShaderRHI, SrcTexture, InSrcTexture);
		if (OutTexture.IsBound())
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutTexture.GetBaseIndex(), InOutUAV);

		FSimpleUniformStructParameters ShaderStructData;
		ShaderStructData.Color1 = UniformStruct.Color1;
		ShaderStructData.Color2 = UniformStruct.Color2;
		ShaderStructData.Color3 = UniformStruct.Color3;
		ShaderStructData.Color4 = UniformStruct.Color4;
		ShaderStructData.ColorIndex = UniformStruct.ColorIndex;

		SetUniformBufferParameterImmediate(RHICmdList, GetComputeShader(), GetUniformBufferParameter<FSimpleUniformStructParameters>(), ShaderStructData);
	}

	/*
	* Unbinds any buffers that have been bound.
	*/
	void UnbindBuffers(FRHICommandList& RHICmdList)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (OutTexture.IsBound())
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutTexture.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
	}
protected:
	FShaderResourceParameter OutTexture;
};

IMPLEMENT_SHADER_TYPE(, FSimpleComputeShader, TEXT("/Plugin/BRPlugins/Private/SimpleComputeShader.usf"), TEXT("MainCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FSimplePixelShaderVS, TEXT("/Plugin/BRPlugins/Private/SimplePixelShader.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, FSimplePixelShaderPS, TEXT("/Plugin/BRPlugins/Private/SimplePixelShader.usf"), TEXT("MainPS"), SF_Pixel)

static void DrawTestShaderRenderTarget_RenderThread(  
    FRHICommandListImmediate& RHICmdList,   
    FTextureRenderTargetResource* OutputRenderTargetResource,  
    ERHIFeatureLevel::Type FeatureLevel,  
//    const FName TextureRenderTargetName,  
    const FLinearColor MyColor,
	const FTextureRHIParamRef TextureRHI,
	const FSimpleUniformStruct UniformStruct
)  
{  
    check(IsInRenderingThread());  
 
#if WANTS_DRAW_MESH_EVENTS  
	SCOPED_DRAW_EVENTF(RHICmdList, SceneCapture, TEXT("SimplePixelShaderPassTest"));
#else  
    SCOPED_DRAW_EVENT(RHICmdList, DrawTestShaderRenderTarget_RenderThread);  
#endif  
	FRHIRenderPassInfo RPInfo(OutputRenderTargetResource->GetRenderTargetTexture(), ERenderTargetActions::DontLoad_Store, OutputRenderTargetResource->TextureRHI);
	RHICmdList.BeginRenderPass(RPInfo, TEXT("SimplePixelShaderPass"));

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
		Parameters.Color1 = UniformStruct.Color1;
		Parameters.Color2 = UniformStruct.Color2;
		Parameters.Color3 = UniformStruct.Color3;
		Parameters.Color4 = UniformStruct.Color4;
		Parameters.ColorIndex= UniformStruct.ColorIndex;

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

static void UseComputeShader_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	FTextureRenderTargetResource* OutputRenderTargetResource,
	FSimpleUniformStruct UniformStruct,
	ERHIFeatureLevel::Type FeatureLevel
)
{
	check(IsInRenderingThread());

	TShaderMapRef<FSimpleComputeShader> ComputeShader(GetGlobalShaderMap(FeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());

	//ComputeShader->SetSurfaces(RHICmdList,)  
	int32 SizeX = OutputRenderTargetResource->GetSizeX();
	int32 SizeY = OutputRenderTargetResource->GetSizeY();
	FRHIResourceCreateInfo CreateInfo;

	FTexture2DRHIRef Texture = RHICreateTexture2D(SizeX, SizeY, PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
	FUnorderedAccessViewRHIRef TextureUAV = RHICreateUnorderedAccessView(Texture);
	ComputeShader->SetParameters(RHICmdList, TextureUAV, UniformStruct);
	DispatchComputeShader(RHICmdList, *ComputeShader, SizeX / 32, SizeY / 32, 1);
	ComputeShader->UnbindBuffers(RHICmdList);

	DrawTestShaderRenderTarget_RenderThread(RHICmdList, OutputRenderTargetResource, FeatureLevel, FLinearColor(), Texture, UniformStruct);
}
 
USimplePixelShaderBlueprintLibrary::USimplePixelShaderBlueprintLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{ }

void USimplePixelShaderBlueprintLibrary::DrawTestShaderRenderTarget(const UObject* WorldContextObject, UTextureRenderTarget2D* OutputRenderTarget, FLinearColor MyColor, UTexture* MyTexture, FSimpleUniformStruct UniformStruct)
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
  /*  FName TextureRenderTargetName = OutputRenderTarget->GetFName(); */ 
    ENQUEUE_RENDER_COMMAND(CaptureCommand)(  
        [TextureRenderTargetResource, FeatureLevel, MyColor,TextureRHI, UniformStruct](FRHICommandListImmediate& RHICmdList)
        {  
            DrawTestShaderRenderTarget_RenderThread(RHICmdList,TextureRenderTargetResource, FeatureLevel, MyColor,TextureRHI, UniformStruct);
        }  
    );  
}  

void USimplePixelShaderBlueprintLibrary::TextureWriting(UTexture2D* TextureToBeWrite)
{
	check(IsInGameThread());

	if (TextureToBeWrite == nullptr)return;
	
	//保存原式贴图设置以便之后恢复
	TextureCompressionSettings OldCompressionSettings = TextureToBeWrite->CompressionSettings;
	TextureMipGenSettings OldMipGenSettings = TextureToBeWrite->MipGenSettings;
	uint8 OldSRGB = TextureToBeWrite->SRGB;
	
	//设置新的格式，不然Mipmap.BulkData.Lock(LOCK_READ_WRITE)会得到空指针
	TextureToBeWrite->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
	TextureToBeWrite->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
	TextureToBeWrite->SRGB = 0;
	TextureToBeWrite->UpdateResource();

	FTexture2DMipMap& Mipmap = TextureToBeWrite->PlatformData->Mips[0];
	void* Data = Mipmap.BulkData.Lock(LOCK_READ_WRITE);

	TArray<FColor> Colors;
	const int PixelNum = TextureToBeWrite->PlatformData->SizeX*TextureToBeWrite->PlatformData->SizeY;
	//Colors.Init(FColor::Red, PixelNum);

	for (int i=0;i<PixelNum;++i)
	{
		Colors.Add(FColor::Red);
	}

	const int32 Stride=(int32)(sizeof(uint8) * 4);
	FMemory::Memcpy(Data, Colors.GetData(), PixelNum*Stride);
	Mipmap.BulkData.Unlock();

	//恢复原本贴图设置
	TextureToBeWrite->CompressionSettings = OldCompressionSettings;
	TextureToBeWrite->MipGenSettings = OldMipGenSettings;
	TextureToBeWrite->SRGB = OldSRGB;

	TextureToBeWrite->Source.Init(TextureToBeWrite->PlatformData->SizeX, TextureToBeWrite->PlatformData->SizeY, 1, 1, ETextureSourceFormat::TSF_BGRA8, (uint8*)Colors.GetData());
	TextureToBeWrite->UpdateResource();
}

void USimplePixelShaderBlueprintLibrary::CreateTexture(const FString& TextureName,const FString& PackageName)
{
	int32 TextureWidth = 1024;
	int32 TextureHeight = 1024;

	FString AssetPath = TEXT("/Game/")+ PackageName+ TEXT("/");
	AssetPath += TextureName;
	UPackage* Package = CreatePackage(NULL, *AssetPath);
	Package->FullyLoad();

	UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *TextureName, RF_Public | RF_Standalone | RF_MarkAsRootSet);

	NewTexture->AddToRoot();				// This line prevents garbage collection of the texture
	NewTexture->PlatformData = new FTexturePlatformData();	// Then we initialize the PlatformData
	NewTexture->PlatformData->SizeX = TextureWidth;
	NewTexture->PlatformData->SizeY = TextureHeight;
	NewTexture->PlatformData->NumSlices = 1;
	NewTexture->PlatformData->PixelFormat = EPixelFormat::PF_B8G8R8A8;

	uint8* Pixels = new uint8[TextureWidth * TextureHeight * 4];
	for (int32 y = 0; y < TextureHeight; y++)
	{
		for (int32 x = 0; x < TextureWidth; x++)
		{
			int32 curPixelIndex = ((y * TextureWidth) + x);
			Pixels[4 * curPixelIndex] = 100;
			Pixels[4 * curPixelIndex + 1] = 50;
			Pixels[4 * curPixelIndex + 2] = 20;
			Pixels[4 * curPixelIndex + 3] = 255;
		}
	}

	// Allocate first mipmap.
	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	NewTexture->PlatformData->Mips.Add(Mip);
	Mip->SizeX = TextureWidth;
	Mip->SizeY = TextureHeight;

	// Lock the texture so it can be modified
	Mip->BulkData.Lock(LOCK_READ_WRITE);
	uint8* TextureData = (uint8*)Mip->BulkData.Realloc(TextureWidth * TextureHeight * 4);
	FMemory::Memcpy(TextureData, Pixels, sizeof(uint8) * TextureHeight * TextureWidth * 4);
	Mip->BulkData.Unlock();

	NewTexture->Source.Init(TextureWidth, TextureHeight, 1, 1, ETextureSourceFormat::TSF_BGRA8, Pixels);

	NewTexture->UpdateResource();
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewTexture);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	bool bSaved = UPackage::SavePackage(Package, NewTexture, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName, GError, nullptr, true, true, SAVE_NoError);

	delete[] Pixels;
}

void USimplePixelShaderBlueprintLibrary::UseComputeShader(const UObject* WorldContextObject, UTextureRenderTarget2D* OutputRenderTarget,FSimpleUniformStruct UniformStruct)
{
	check(IsInGameThread());

	FTextureRenderTargetResource* TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();
	const UWorld* World = WorldContextObject->GetWorld();
	ERHIFeatureLevel::Type FeatureLevel = World->Scene->GetFeatureLevel();

	ENQUEUE_RENDER_COMMAND(CaptureCommand)(
		[TextureRenderTargetResource, FeatureLevel, UniformStruct](FRHICommandListImmediate& RHICmdList)
	{
		UseComputeShader_RenderThread
		(
			RHICmdList,
			TextureRenderTargetResource,
			UniformStruct,
			FeatureLevel
		);
	});
}

#undef LOCTEXT_NAMESPACE  