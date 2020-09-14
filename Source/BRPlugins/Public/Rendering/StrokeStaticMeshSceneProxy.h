#pragma once
#include "Engine\Public\StaticMeshResources.h"

class FStrokeStaticMeshSceneProxy : public FStaticMeshSceneProxy
{
public:
	//在UStrokeStaticMeshComponent中调用这个构造函数初始化
	FStrokeStaticMeshSceneProxy(UStaticMeshComponent* Component, bool bForceLODsShareStaticLighting);

	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override;
	
private:
	UPROPERTY()
	const UStaticMeshComponent* ComponentPtr;
};