#pragma once
#include "CoreMinimal.h"
#include "Engine/Classes/Components/SkeletalMeshComponent.h"
#include "StrokeSkeletalMeshComponent.generated.h"

UCLASS(ClassGroup=(Rendering, Common), hidecategories=Object,  editinlinenew, meta=(BlueprintSpawnableComponent))
class UStrokeSkeletalMeshComponent : public USkeletalMeshComponent
{
	GENERATED_UCLASS_BODY()

	//~ Begin UPrimitiveComponent Interface
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;

	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent Interface

	//virtual UMaterialInterface* GetMaterial(int32 MaterialIndex) const override;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	UMaterialInterface* SecondPassMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	bool NeedSecondPass=false;
};