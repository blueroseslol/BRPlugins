#include "StrokeSkeletalMeshComponent.h"
#include "StrokeSkeletalMeshSceneProxy.h"
#include "Engine/Public/Rendering/SkeletalMeshRenderData.h"
#include "Engine/Public/SkeletalRenderPublic.h"
//#include "Engine/Classes/PhysicsEngine/PhysicsAsset.h"
//#include "Engine/Classes/Materials/MaterialInstance.h"
#define LOCTEXT_NAMESPACE "StrokeSkeletalMeshComponent"

UStrokeSkeletalMeshComponent::UStrokeSkeletalMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UStrokeSkeletalMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials /*= false*/) const
{
	if (SkeletalMesh)
	{
		// The max number of materials used is the max of the materials on the skeletal mesh and the materials on the mesh component
		const int32 NumMaterials = FMath::Max(SkeletalMesh->Materials.Num(), OverrideMaterials.Num());
		for (int32 MatIdx = 0; MatIdx < NumMaterials; ++MatIdx)
		{
			// GetMaterial will determine the correct material to use for this index.  

			UMaterialInterface* MaterialInterface = GetMaterial(MatIdx);
			OutMaterials.Add(MaterialInterface);
		}
		if (NeedSecondPass)
		{
			for (int i = 0; i < SkeletalMesh->Materials.Num(); ++i)
			{
				if (SkeletalMesh->Materials[i].MaterialSlotName == FName("NAME_Stroke"))
				{
					SkeletalMesh->Materials.RemoveAt(i);
				}
			}
			
			SkeletalMesh->Materials.Add(FSkeletalMaterial(SecondPassMaterial,true,false,FName("NAME_Stroke")));
			SkeletalMesh->UpdateUVChannelData(false);
			OutMaterials.Add(SecondPassMaterial);
		}
	}

	if (bGetDebugMaterials)
	{
#if WITH_EDITOR
		//if (UPhysicsAsset* PhysicsAssetForDebug = GetPhysicsAsset())
		//{
		//	PhysicsAssetForDebug->GetUsedMaterials(OutMaterials);
		//}
#endif
	}
}

FPrimitiveSceneProxy* UStrokeSkeletalMeshComponent::CreateSceneProxy()
{
	ERHIFeatureLevel::Type SceneFeatureLevel = GetWorld()->FeatureLevel;

	FStrokeSkeletalMeshSceneProxy* Result = nullptr;
	//FSkeletalMeshSceneProxy* Result = nullptr;
	FSkeletalMeshRenderData* SkelMeshRenderData = GetSkeletalMeshRenderData();

	// Only create a scene proxy for rendering if properly initialized
	if (SkelMeshRenderData &&
		SkelMeshRenderData->LODRenderData.IsValidIndex(PredictedLODLevel) &&
		!bHideSkin &&
		MeshObject)
	{
		// Only create a scene proxy if the bone count being used is supported, or if we don't have a skeleton (this is the case with destructibles)
		int32 MaxBonesPerChunk = SkelMeshRenderData->GetMaxBonesPerSection();
		int32 MaxSupportedNumBones = MeshObject->IsCPUSkinned() ? MAX_int32 : GetFeatureLevelMaxNumberOfBones(SceneFeatureLevel);
		if (MaxBonesPerChunk <= MaxSupportedNumBones)
		{
			Result = ::new FStrokeSkeletalMeshSceneProxy(this, SkelMeshRenderData);
		}
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	SendRenderDebugPhysics(Result);
#endif

	return Result;
}

//UMaterialInterface* UStrokeSkeletalMeshComponent::GetMaterial(int32 MaterialIndex) const
//{
//	if (OverrideMaterials.IsValidIndex(MaterialIndex) && OverrideMaterials[MaterialIndex])
//	{
//		return OverrideMaterials[MaterialIndex];
//	}
//	else if (SkeletalMesh && SkeletalMesh->Materials.IsValidIndex(MaterialIndex) && SkeletalMesh->Materials[MaterialIndex].MaterialInterface)
//	{
//		return SkeletalMesh->Materials[MaterialIndex].MaterialInterface;
//	}
//	return nullptr;
//}

#undef LOCTEXT_NAMESPACE