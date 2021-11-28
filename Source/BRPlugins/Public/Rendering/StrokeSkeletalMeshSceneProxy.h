#pragma once
#include "Engine/Public/SkeletalMeshTypes.h"

class FStrokeSkeletalMeshSceneProxy : public FSkeletalMeshSceneProxy
{
public:
	//在UStrokeSkeletalMeshComponent中调用这个构造函数初始化
	FStrokeSkeletalMeshSceneProxy(const USkinnedMeshComponent* Component, FSkeletalMeshRenderData* InSkelMeshRenderData);

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

	void GetDynamicElementsSection(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, 
		const FSkeletalMeshLODRenderData& LODData, const int32 LODIndex, const int32 SectionIndex, bool bSectionSelected,
		const FSectionElementInfo& SectionElementInfo, bool bInSelectable, FMeshElementCollector& Collector ) const;
	
private:
	void CreateBaseMeshBatch(const FSceneView* View, const FSkeletalMeshLODRenderData& LODData, const int32 LODIndex, const int32 SectionIndex, const FSectionElementInfo& SectionElementInfo, FMeshBatch& Mesh) const;
	
	UPROPERTY()
	const USkinnedMeshComponent* ComponentPtr;
};