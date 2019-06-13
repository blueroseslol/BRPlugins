#pragma once
#include "CoreMinimal.h"
#include "Engine/Classes/Components/StaticMeshComponent.h"
#include "StrokeStaticMeshComponent.generated.h"

UCLASS(ClassGroup=(Rendering, Common), hidecategories=Object,  editinlinenew, meta=(BlueprintSpawnableComponent))
class UStrokeStaticMeshComponent : public UStaticMeshComponent
{
	GENERATED_UCLASS_BODY()

	//~ Begin UPrimitiveComponent Interface
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;

	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent Interface
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	UMaterialInterface* SecondPassMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	bool NeedSecondPass=false;
};