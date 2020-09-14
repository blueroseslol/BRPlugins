#pragma once
#include "Engine/Public/SkeletalMeshTypes.h"

class FStrokeSkeletalMeshSceneProxy : public FSkeletalMeshSceneProxy
{
public:
	//在UStrokeSkeletalMeshComponent中调用这个构造函数初始化
	FStrokeSkeletalMeshSceneProxy(const USkinnedMeshComponent* Component, FSkeletalMeshRenderData* InSkelMeshRenderData);

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
private:
	UPROPERTY()
	const USkinnedMeshComponent* ComponentPtr;
};