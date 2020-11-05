#include "Rendering/StrokeStaticMeshComponent.h"
#include "Rendering/StrokeStaticMeshSceneProxy.h"
#include "Engine/Public/Rendering/SkeletalMeshRenderData.h"
#include "Engine/Public/SkeletalRenderPublic.h"
#define LOCTEXT_NAMESPACE "StrokeSkeletalMeshComponent"

UStrokeStaticMeshComponent::UStrokeStaticMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UStrokeStaticMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials /*= false*/) const
{
	if (GetStaticMesh() && GetStaticMesh()->RenderData)
	{
		TMap<int32, UMaterialInterface*> MapOfMaterials;
		for (int32 LODIndex = 0; LODIndex < GetStaticMesh()->RenderData->LODResources.Num(); LODIndex++)
		{
			FStaticMeshLODResources& LODResources = GetStaticMesh()->RenderData->LODResources[LODIndex];
			int32 MaterialNum = 0;
			for (int32 SectionIndex = 0; SectionIndex < LODResources.Sections.Num(); SectionIndex++)
			{
				// Get the material for each element at the current lod index
				int32 MaterialIndex = LODResources.Sections[SectionIndex].MaterialIndex;
				if (!MapOfMaterials.Contains(MaterialIndex))
				{
					MapOfMaterials.Add(MaterialIndex, GetMaterial(MaterialIndex));
					MaterialNum++;
				}
			}

			if (NeedSecondPass)
			{
				bool NeedAddMaterial = true;
				for (int i = 0; i < MapOfMaterials.Num(); ++i)
				{
					if (MapOfMaterials[i]== SecondPassMaterial)
					{
						NeedAddMaterial = false;
					}
				}
				if (NeedAddMaterial)
				{
					MapOfMaterials.Add(MaterialNum, SecondPassMaterial);
				}
			}
		}
		if (MapOfMaterials.Num() > 0)
		{
			//We need to output the material in the correct order (follow the material index)
			//So we sort the map with the material index
			MapOfMaterials.KeySort([](int32 A, int32 B) {
				return A < B; // sort keys in order
			});

			//Preadd all the material item in the array
			OutMaterials.AddZeroed(MapOfMaterials.Num());
			//Set the value in the correct order
			int32 MaterialIndex = 0;
			for (auto Kvp : MapOfMaterials)
			{
				OutMaterials[MaterialIndex++] = Kvp.Value;
			}
		}
	}
}

FPrimitiveSceneProxy* UStrokeStaticMeshComponent::CreateSceneProxy()
{
	if (GetStaticMesh() == nullptr || GetStaticMesh()->RenderData == nullptr)
	{
		return nullptr;
	}

	const TIndirectArray<FStaticMeshLODResources>& LODResources = GetStaticMesh()->RenderData->LODResources;
	if (LODResources.Num() == 0 || LODResources[FMath::Clamp<int32>(GetStaticMesh()->MinLOD.Default, 0, LODResources.Num() - 1)].VertexBuffers.StaticMeshVertexBuffer.GetNumVertices() == 0)
	{
		return nullptr;
	}

	FPrimitiveSceneProxy* Proxy = ::new FStrokeStaticMeshSceneProxy(this, false);
#if STATICMESH_ENABLE_DEBUG_RENDERING
	SendRenderDebugPhysics(Proxy);
#endif

	return Proxy;
}

#undef LOCTEXT_NAMESPACE