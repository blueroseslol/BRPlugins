#include "Rendering/StrokeSkeletalMeshSceneProxy.h"
#include "Engine/Public/SkeletalMeshTypes.h"
#include "Engine/Public/TessellationRendering.h"
#include "Engine/Public/SkeletalRenderPublic.h"
#include "Rendering/StrokeSkeletalMeshObjectGPUSkin.h"
#define LOCTEXT_NAMESPACE "StrokeSkeletalMeshSceneProxy"

FStrokeSkeletalMeshSceneProxy::FStrokeSkeletalMeshSceneProxy(const USkinnedMeshComponent* Component, FSkeletalMeshRenderData* InSkelMeshRenderData):
	FSkeletalMeshSceneProxy(Component,InSkelMeshRenderData),ComponentPtr(Component)
{
}

class FSkeletalMeshSectionIter
{
public:
	FSkeletalMeshSectionIter(const int32 InLODIdx, const FSkeletalMeshObject& InMeshObject, const FSkeletalMeshLODRenderData& InLODData, const FSkeletalMeshSceneProxy::FLODSectionElements& InLODSectionElements)
		: SectionIndex(0)
		, MeshObject(InMeshObject)
		, LODSectionElements(InLODSectionElements)
		, Sections(InLODData.RenderSections)
#if WITH_EDITORONLY_DATA
		, SectionIndexPreview(InMeshObject.SectionIndexPreview)
		, MaterialIndexPreview(InMeshObject.MaterialIndexPreview)
#endif
	{
		while (NotValidPreviewSection())
		{
			SectionIndex++;
		}
	}
	FORCEINLINE FSkeletalMeshSectionIter& operator++()
	{
		do
		{
			SectionIndex++;
		} while (NotValidPreviewSection());
		return *this;
	}
	FORCEINLINE operator bool() const
	{
		return ((SectionIndex < Sections.Num()) && LODSectionElements.SectionElements.IsValidIndex(GetSectionElementIndex()));
	}
	FORCEINLINE const FSkelMeshRenderSection& GetSection() const
	{
		return Sections[SectionIndex];
	}
	FORCEINLINE const int32 GetSectionElementIndex() const
	{
		return SectionIndex;
	}
	FORCEINLINE const FSkeletalMeshSceneProxy::FSectionElementInfo& GetSectionElementInfo() const
	{
		int32 SectionElementInfoIndex = GetSectionElementIndex();
		return LODSectionElements.SectionElements[SectionElementInfoIndex];
	}
	FORCEINLINE bool NotValidPreviewSection()
	{
#if WITH_EDITORONLY_DATA
		if (MaterialIndexPreview == INDEX_NONE)
		{
			int32 ActualPreviewSectionIdx = SectionIndexPreview;

			return	(SectionIndex < Sections.Num()) &&
				((ActualPreviewSectionIdx >= 0) && (ActualPreviewSectionIdx != SectionIndex));
		}
		else
		{
			int32 ActualPreviewMaterialIdx = MaterialIndexPreview;
			int32 ActualPreviewSectionIdx = INDEX_NONE;
			if (ActualPreviewMaterialIdx != INDEX_NONE && Sections.IsValidIndex(SectionIndex))
			{
				const FSkeletalMeshSceneProxy::FSectionElementInfo& SectionInfo = LODSectionElements.SectionElements[SectionIndex];
				if (SectionInfo.UseMaterialIndex == ActualPreviewMaterialIdx)
				{
					ActualPreviewSectionIdx = SectionIndex;
				}
			}

			return	(SectionIndex < Sections.Num()) &&
				((ActualPreviewMaterialIdx >= 0) && (ActualPreviewSectionIdx != SectionIndex));
		}
#else
		return false;
#endif
	}
private:
	int32 SectionIndex;
	const FSkeletalMeshObject& MeshObject;
	const FSkeletalMeshSceneProxy::FLODSectionElements& LODSectionElements;
	const TArray<FSkelMeshRenderSection>& Sections;
#if WITH_EDITORONLY_DATA
	const int32 SectionIndexPreview;
	const int32 MaterialIndexPreview;
#endif
};

void FStrokeSkeletalMeshSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FStrokeSkeletalMeshSceneProxy_GetMeshElements);
	
	if (!MeshObject)
	{
		return;
	}
	MeshObject->PreGDMECallback(ViewFamily.Scene->GetGPUSkinCache(), ViewFamily.FrameNumber);

	const FEngineShowFlags& EngineShowFlags = ViewFamily.EngineShowFlags;

	const int32 LODIndex = MeshObject->GetLOD();
	check(LODIndex < SkeletalMeshRenderData->LODRenderData.Num());
	const FSkeletalMeshLODRenderData& LODData = SkeletalMeshRenderData->LODRenderData[LODIndex];

	if (LODSections.Num() > 0)
	{
		const FLODSectionElements& LODSection = LODSections[LODIndex];

		check(LODSection.SectionElements.Num() == LODData.RenderSections.Num());

		for (FSkeletalMeshSectionIter Iter(LODIndex, *MeshObject, LODData, LODSection); Iter; ++Iter)
		{
			const FSkelMeshRenderSection& Section = Iter.GetSection();
			const int32 SectionIndex = Iter.GetSectionElementIndex();
			const FSectionElementInfo& SectionElementInfo = Iter.GetSectionElementInfo();

			bool bSectionSelected = false;

#if WITH_EDITORONLY_DATA
			// TODO: This is not threadsafe! A render command should be used to propagate SelectedEditorSection to the scene proxy.
			if (MeshObject->SelectedEditorMaterial != INDEX_NONE)
			{
				bSectionSelected = (MeshObject->SelectedEditorMaterial == SectionElementInfo.UseMaterialIndex);
			}
			else
			{
				bSectionSelected = (MeshObject->SelectedEditorSection == SectionIndex);
			}

#endif
			// If hidden skip the draw
			check(MeshObject->LODInfo.IsValidIndex(LODIndex));
			bool bHide= MeshObject->LODInfo[LODIndex].HiddenMaterials.IsValidIndex(SectionElementInfo.UseMaterialIndex) && MeshObject->LODInfo[LODIndex].HiddenMaterials[SectionElementInfo.UseMaterialIndex];

			if (bHide || Section.bDisabled)
			{
				continue;
			}

			/* error LNK2019
			if (MeshObject->IsMaterialHidden(LODIndex, SectionElementInfo.UseMaterialIndex) || Section.bDisabled)
			{
				continue;
			}
			*/
			const UStrokeSkeletalMeshComponent* StrokeSkeletalMeshComponent = dynamic_cast<const UStrokeSkeletalMeshComponent *>(ComponentPtr);
			if (SectionElementInfo.Material== StrokeSkeletalMeshComponent->SecondPassMaterial)
			{
				continue;
			}
			FSkeletalMeshSceneProxy::GetDynamicElementsSection(Views, ViewFamily, VisibilityMap, LODData, LODIndex, SectionIndex, bSectionSelected, SectionElementInfo, true, Collector);
			
			//轮廓
			if (StrokeSkeletalMeshComponent->NeedSecondPass) {
				// FSectionElementInfo Info = FSectionElementInfo(SectionElementInfo);
				TSharedPtr<FSectionElementInfo> Info=MakeShared<FSectionElementInfo>(FSectionElementInfo(SectionElementInfo));
				if (StrokeSkeletalMeshComponent->SecondPassMaterial == nullptr) {
					continue;
				}
				Info->Material = StrokeSkeletalMeshComponent->SecondPassMaterial;
				FStrokeSkeletalMeshSceneProxy::GetDynamicElementsSection(Views, ViewFamily, VisibilityMap, LODData, LODIndex, SectionIndex, bSectionSelected, *Info.Get(), true, Collector);
			}
		}
	}
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			if (PhysicsAssetForDebug)
			{
				DebugDrawPhysicsAsset(ViewIndex, Collector, ViewFamily.EngineShowFlags);
			}

			if (EngineShowFlags.MassProperties && DebugMassData.Num() > 0)
			{
				FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
				if (MeshObject->GetComponentSpaceTransforms())
				{
					const TArray<FTransform>& ComponentSpaceTransforms = *MeshObject->GetComponentSpaceTransforms();

					for (const FDebugMassData& DebugMass : DebugMassData)
					{
						if (ComponentSpaceTransforms.IsValidIndex(DebugMass.BoneIndex))
						{
							const FTransform BoneToWorld = ComponentSpaceTransforms[DebugMass.BoneIndex] * FTransform(GetLocalToWorld());
							DebugMass.DrawDebugMass(PDI, BoneToWorld);
						}
					}
				}
			}

			if (ViewFamily.EngineShowFlags.SkeletalMeshes)
			{
				RenderBounds(Collector.GetPDI(ViewIndex), ViewFamily.EngineShowFlags, GetBounds(), IsSelected());
			}

			if (ViewFamily.EngineShowFlags.Bones)
			{
				DebugDrawSkeleton(ViewIndex, Collector, ViewFamily.EngineShowFlags);
			}
		}
	}
#endif
}

void FStrokeSkeletalMeshSceneProxy::GetDynamicElementsSection(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, 
	const FSkeletalMeshLODRenderData& LODData, const int32 LODIndex, const int32 SectionIndex, bool bSectionSelected,
	const FSectionElementInfo& SectionElementInfo, bool bInSelectable, FMeshElementCollector& Collector ) const
{
	const FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIndex];

	//// If hidden skip the draw
	//if (Section.bDisabled || MeshObject->IsMaterialHidden(LODIndex,SectionElementInfo.UseMaterialIndex))
	//{
	//	return;
	//}

#if !WITH_EDITOR
	const bool bIsSelected = false;
#else // #if !WITH_EDITOR
	bool bIsSelected = IsSelected();

	// if the mesh isn't selected but the mesh section is selected in the AnimSetViewer, find the mesh component and make sure that it can be highlighted (ie. are we rendering for the AnimSetViewer or not?)
	if( !bIsSelected && bSectionSelected && bCanHighlightSelectedSections )
	{
		bIsSelected = true;
	}
#endif // #if WITH_EDITOR

	const bool bIsWireframe = ViewFamily.EngineShowFlags.Wireframe;

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			const FSceneView* View = Views[ViewIndex];

			FMeshBatch& Mesh = Collector.AllocateMesh();

			CreateBaseMeshBatch(View, LODData, LODIndex, SectionIndex, SectionElementInfo, Mesh);
			
			if(!Mesh.VertexFactory)
			{
				// hide this part
				continue;
			}

			Mesh.bWireframe |= bForceWireframe;
			Mesh.Type = PT_TriangleList;
			Mesh.bSelectable = bInSelectable;

			FMeshBatchElement& BatchElement = Mesh.Elements[0];
			const bool bRequiresAdjacencyInformation = RequiresAdjacencyInformation( SectionElementInfo.Material, Mesh.VertexFactory->GetType(), ViewFamily.GetFeatureLevel() );
			if ( bRequiresAdjacencyInformation )
			{
				check(LODData.AdjacencyMultiSizeIndexContainer.IsIndexBufferValid() );
				BatchElement.IndexBuffer = LODData.AdjacencyMultiSizeIndexContainer.GetIndexBuffer();
				Mesh.Type = PT_12_ControlPointPatchList;
				BatchElement.FirstIndex *= 4;
			}

		#if WITH_EDITOR
			Mesh.BatchHitProxyId = SectionElementInfo.HitProxy ? SectionElementInfo.HitProxy->Id : FHitProxyId();

			if (bSectionSelected && bCanHighlightSelectedSections)
			{
				Mesh.bUseSelectionOutline = true;
			}
			else
			{
				Mesh.bUseSelectionOutline = !bCanHighlightSelectedSections && bIsSelected;
			}
		#endif

#if WITH_EDITORONLY_DATA
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (bIsSelected)
			{
				if (ViewFamily.EngineShowFlags.VertexColors && AllowDebugViewmodes())
				{
					// Override the mesh's material with our material that draws the vertex colors
					UMaterial* VertexColorVisualizationMaterial = NULL;
					switch (GVertexColorViewMode)
					{
					case EVertexColorViewMode::Color:
						VertexColorVisualizationMaterial = GEngine->VertexColorViewModeMaterial_ColorOnly;
						break;

					case EVertexColorViewMode::Alpha:
						VertexColorVisualizationMaterial = GEngine->VertexColorViewModeMaterial_AlphaAsColor;
						break;

					case EVertexColorViewMode::Red:
						VertexColorVisualizationMaterial = GEngine->VertexColorViewModeMaterial_RedOnly;
						break;

					case EVertexColorViewMode::Green:
						VertexColorVisualizationMaterial = GEngine->VertexColorViewModeMaterial_GreenOnly;
						break;

					case EVertexColorViewMode::Blue:
						VertexColorVisualizationMaterial = GEngine->VertexColorViewModeMaterial_BlueOnly;
						break;
					}
					check(VertexColorVisualizationMaterial != NULL);

					auto VertexColorVisualizationMaterialInstance = new FColoredMaterialRenderProxy(
						VertexColorVisualizationMaterial->GetRenderProxy(),
						GetSelectionColor(FLinearColor::White, bSectionSelected, IsHovered())
					);

					Collector.RegisterOneFrameMaterialProxy(VertexColorVisualizationMaterialInstance);
					Mesh.MaterialRenderProxy = VertexColorVisualizationMaterialInstance;
				}
			}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#endif // WITH_EDITORONLY_DATA

			BatchElement.MinVertexIndex = Section.BaseVertexIndex;
			Mesh.CastShadow = SectionElementInfo.bEnableShadowCasting;
			Mesh.bCanApplyViewModeOverrides = true;
			Mesh.bUseWireframeSelectionColoring = bIsSelected;
			Mesh.ReverseCulling = true;
			
		#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			BatchElement.VisualizeElementIndex = SectionIndex;
			Mesh.VisualizeLODIndex = LODIndex;
		#endif

			if (ensureMsgf(Mesh.MaterialRenderProxy, TEXT("GetDynamicElementsSection with invalid MaterialRenderProxy. Owner:%s LODIndex:%d UseMaterialIndex:%d"), *GetOwnerName().ToString(), LODIndex, SectionElementInfo.UseMaterialIndex))
			{
				Collector.AddMesh(ViewIndex, Mesh);
			}

			// const int32 NumVertices = Section.GetNumVertices();
			// INC_DWORD_STAT_BY(STAT_GPUSkinVertices,(uint32)(bIsCPUSkinned ? 0 : NumVertices));
			// INC_DWORD_STAT_BY(STAT_SkelMeshTriangles,Mesh.GetNumPrimitives());
			// INC_DWORD_STAT(STAT_SkelMeshDrawCalls);
		}
	}
}

void FStrokeSkeletalMeshSceneProxy::CreateBaseMeshBatch(const FSceneView* View, const FSkeletalMeshLODRenderData& LODData, const int32 LODIndex, const int32 SectionIndex, const FSectionElementInfo& SectionElementInfo, FMeshBatch& Mesh) const
{
	Mesh.VertexFactory = MeshObject->GetSkinVertexFactory(View, LODIndex, SectionIndex);
	Mesh.MaterialRenderProxy = SectionElementInfo.Material->GetRenderProxy();
#if RHI_RAYTRACING
	Mesh.SegmentIndex = SectionIndex;
	Mesh.CastRayTracedShadow = SectionElementInfo.bEnableShadowCasting && bCastDynamicShadow;
#endif

	FMeshBatchElement& BatchElement = Mesh.Elements[0];
	BatchElement.FirstIndex = LODData.RenderSections[SectionIndex].BaseIndex;
	BatchElement.IndexBuffer = LODData.MultiSizeIndexContainer.GetIndexBuffer();
	BatchElement.MinVertexIndex = LODData.RenderSections[SectionIndex].GetVertexBufferIndex();
	BatchElement.MaxVertexIndex = LODData.RenderSections[SectionIndex].GetVertexBufferIndex() + LODData.RenderSections[SectionIndex].GetNumVertices() - 1;
	
	FStrokeSkeletalMeshObjectGPUSkin* MeshObjectGPUSkin=static_cast<FStrokeSkeletalMeshObjectGPUSkin*>(MeshObject);
	FGPUSkinCacheEntry* GPUSkinCacheEntry=MeshObjectGPUSkin->GetGPUSkinCacheEntry();
	
	BatchElement.VertexFactoryUserData = GPUSkinCacheEntry ? &GPUSkinCacheEntry->BatchElementsUserData[SectionIndex] : nullptr;
	BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();
	BatchElement.NumPrimitives = LODData.RenderSections[SectionIndex].NumTriangles;
}

#undef LOCTEXT_NAMESPACE