#include "Rendering/StrokeStaticMeshSceneProxy.h"
#define LOCTEXT_NAMESPACE "StrokeStaticMeshSceneProxy"

static bool GUseShadowIndexBuffer = true;

FStrokeStaticMeshSceneProxy::FStrokeStaticMeshSceneProxy(UStaticMeshComponent* Component, bool bForceLODsShareStaticLighting):FStaticMeshSceneProxy(Component,bForceLODsShareStaticLighting),ComponentPtr(Component)
{
	
}

// use for render thread only
bool UseLightPropagationVolumeRT2(ERHIFeatureLevel::Type InFeatureLevel)
{
	if (InFeatureLevel < ERHIFeatureLevel::SM5)
	{
		return false;
	}

	// todo: better we get the engine LPV state not the cvar (later we want to make it changeable at runtime)
	static const auto* CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.LightPropagationVolume"));
	check(CVar);

	int32 Value = CVar->GetValueOnRenderThread();

	return Value != 0;
}

inline bool AllowShadowOnlyMesh(ERHIFeatureLevel::Type InFeatureLevel)
{
	// todo: later we should refine that (only if occlusion feature in LPV is on, only if inside a cascade, if shadow casting is disabled it should look at bUseEmissiveForDynamicAreaLighting)
	return !UseLightPropagationVolumeRT2(InFeatureLevel);
}

void FStrokeStaticMeshSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI)
{
	checkSlow(IsInParallelRenderingThread());
	if (!HasViewDependentDPG())
	{
		// Determine the DPG the primitive should be drawn in.
		uint8 PrimitiveDPG = GetStaticDepthPriorityGroup();
		int32 NumLODs = RenderData->LODResources.Num();
		//Never use the dynamic path in this path, because only unselected elements will use DrawStaticElements
		bool bIsMeshElementSelected = false;
		const auto FeatureLevel = GetScene().GetFeatureLevel();

		//check if a LOD is being forced
		if (ForcedLodModel > 0)
		{
			int32 LODIndex = FMath::Clamp(ForcedLodModel, ClampedMinLOD + 1, NumLODs) - 1;
			const FStaticMeshLODResources& LODModel = RenderData->LODResources[LODIndex];
			// Draw the static mesh elements.
			for (int32 SectionIndex = 0; SectionIndex < LODModel.Sections.Num(); SectionIndex++)
			{
#if WITH_EDITOR
				if (GIsEditor)
				{
					const FLODInfo::FSectionInfo& Section = LODs[LODIndex].Sections[SectionIndex];

					bIsMeshElementSelected = Section.bSelected;
					PDI->SetHitProxy(Section.HitProxy);
				}
#endif // WITH_EDITOR

				const int32 NumBatches = GetNumMeshBatches();

				PDI->ReserveMemoryForMeshes(NumBatches);

				for (int32 BatchIndex = 0; BatchIndex < NumBatches; BatchIndex++)
				{
					FMeshBatch MeshBatch;

					if (GetMeshElement(LODIndex, BatchIndex, SectionIndex, PrimitiveDPG, bIsMeshElementSelected, true, MeshBatch))
					{
						PDI->DrawMesh(MeshBatch, FLT_MAX);

						const FLODInfo& ProxyLODInfo = LODs[LODIndex];
						UMaterialInterface* MaterialInterface = ProxyLODInfo.Sections[SectionIndex].Material;

						const UStrokeStaticMeshComponent* StrokeStaticMeshComponent = dynamic_cast<const UStrokeStaticMeshComponent *>(ComponentPtr);
						if (MaterialInterface == StrokeStaticMeshComponent->SecondPassMaterial)
						{
							continue;
						}
						if (StrokeStaticMeshComponent->NeedSecondPass) {
							if (StrokeStaticMeshComponent->SecondPassMaterial == nullptr) {
								continue;
							}
							MeshBatch.MaterialRenderProxy = StrokeStaticMeshComponent->SecondPassMaterial->GetRenderProxy();
							MeshBatch.ReverseCulling = true;
							PDI->DrawMesh(MeshBatch, FLT_MAX);
						}
					}
				}
			}
		}
		else //no LOD is being forced, submit them all with appropriate cull distances
		{
			for (int32 LODIndex = ClampedMinLOD; LODIndex < NumLODs; LODIndex++)
			{
				const FStaticMeshLODResources& LODModel = RenderData->LODResources[LODIndex];
				float ScreenSize = GetScreenSize(LODIndex);

				bool bUseUnifiedMeshForShadow = false;
				bool bUseUnifiedMeshForDepth = false;

				if (GUseShadowIndexBuffer && LODModel.bHasDepthOnlyIndices)
				{
					const FLODInfo& LODInfo = LODs[LODIndex];

					// The shadow-only mesh can be used only if all elements cast shadows and use opaque materials with no vertex modification.
					// In some cases (e.g. LPV) we don't want the optimization
					bool bSafeToUseUnifiedMesh = AllowShadowOnlyMesh(FeatureLevel);

					bool bAnySectionUsesDitheredLODTransition = false;
					bool bAllSectionsUseDitheredLODTransition = true;
					bool bIsMovable = IsMovable();
					bool bAllSectionsCastShadow = bCastShadow;

					for (int32 SectionIndex = 0; bSafeToUseUnifiedMesh && SectionIndex < LODModel.Sections.Num(); SectionIndex++)
					{
						const FMaterial* Material = LODInfo.Sections[SectionIndex].Material->GetRenderProxy()->GetMaterial(FeatureLevel);
						// no support for stateless dithered LOD transitions for movable meshes
						bAnySectionUsesDitheredLODTransition = bAnySectionUsesDitheredLODTransition || (!bIsMovable && Material->IsDitheredLODTransition());
						bAllSectionsUseDitheredLODTransition = bAllSectionsUseDitheredLODTransition && (!bIsMovable && Material->IsDitheredLODTransition());
						const FStaticMeshSection& Section = LODModel.Sections[SectionIndex];

						bSafeToUseUnifiedMesh =
							!(bAnySectionUsesDitheredLODTransition && !bAllSectionsUseDitheredLODTransition) // can't use a single section if they are not homogeneous
							&& Material->WritesEveryPixel()
							&& !Material->IsTwoSided()
							&& !IsTranslucentBlendMode(Material->GetBlendMode())
							&& !Material->MaterialModifiesMeshPosition_RenderThread()
							&& Material->GetMaterialDomain() == MD_Surface;

						bAllSectionsCastShadow &= Section.bCastShadow;
					}

					if (bSafeToUseUnifiedMesh)
					{
						bUseUnifiedMeshForShadow = bAllSectionsCastShadow;

						// Depth pass is only used for deferred renderer. The other conditions are meant to match the logic in FDepthPassMeshProcessor::AddMeshBatch.
						bUseUnifiedMeshForDepth = ShouldUseAsOccluder() && GetScene().GetShadingPath() == EShadingPath::Deferred && !IsMovable();

						if (bUseUnifiedMeshForShadow || bUseUnifiedMeshForDepth)
						{
							const int32 NumBatches = GetNumMeshBatches();

							PDI->ReserveMemoryForMeshes(NumBatches);

							for (int32 BatchIndex = 0; BatchIndex < NumBatches; BatchIndex++)
							{
								FMeshBatch MeshBatch;

								if (GetShadowMeshElement(LODIndex, BatchIndex, PrimitiveDPG, MeshBatch, bAllSectionsUseDitheredLODTransition))
								{
									bUseUnifiedMeshForShadow = bAllSectionsCastShadow;

									MeshBatch.CastShadow = bUseUnifiedMeshForShadow;
									MeshBatch.bUseForDepthPass = bUseUnifiedMeshForDepth;
									MeshBatch.bUseAsOccluder = bUseUnifiedMeshForDepth;
									MeshBatch.bUseForMaterial = false;

									PDI->DrawMesh(MeshBatch, ScreenSize);

									const FLODInfo& ProxyLODInfo = LODs[LODIndex];
									UMaterialInterface* MaterialInterface = ProxyLODInfo.Sections[0].Material;

									const UStrokeStaticMeshComponent* StrokeStaticMeshComponent = dynamic_cast<const UStrokeStaticMeshComponent *>(ComponentPtr);
									if (MaterialInterface == StrokeStaticMeshComponent->SecondPassMaterial)
									{
										continue;
									}
									if (StrokeStaticMeshComponent->NeedSecondPass) {
										if (StrokeStaticMeshComponent->SecondPassMaterial == nullptr) {
											continue;
										}
										MeshBatch.MaterialRenderProxy = StrokeStaticMeshComponent->SecondPassMaterial->GetRenderProxy();
										MeshBatch.ReverseCulling = true;
										PDI->DrawMesh(MeshBatch, FLT_MAX);
									}
								}
							}
						}
					}
				}

				// Draw the static mesh elements.
				for (int32 SectionIndex = 0; SectionIndex < LODModel.Sections.Num(); SectionIndex++)
				{
#if WITH_EDITOR
					if (GIsEditor)
					{
						const FLODInfo::FSectionInfo& Section = LODs[LODIndex].Sections[SectionIndex];

						bIsMeshElementSelected = Section.bSelected;
						PDI->SetHitProxy(Section.HitProxy);
					}
#endif // WITH_EDITOR

					const int32 NumBatches = GetNumMeshBatches();

					PDI->ReserveMemoryForMeshes(NumBatches);

					for (int32 BatchIndex = 0; BatchIndex < NumBatches; BatchIndex++)
					{
						FMeshBatch MeshBatch;

						if (GetMeshElement(LODIndex, BatchIndex, SectionIndex, PrimitiveDPG, bIsMeshElementSelected, true, MeshBatch))
						{
							// If we have submitted an optimized shadow-only mesh, remaining mesh elements must not cast shadows.
							MeshBatch.CastShadow &= !bUseUnifiedMeshForShadow;
							MeshBatch.bUseAsOccluder &= !bUseUnifiedMeshForDepth;
							MeshBatch.bUseForDepthPass &= !bUseUnifiedMeshForDepth;

							PDI->DrawMesh(MeshBatch, ScreenSize);

							const FLODInfo& ProxyLODInfo = LODs[LODIndex];
							UMaterialInterface* MaterialInterface = ProxyLODInfo.Sections[SectionIndex].Material;

							const UStrokeStaticMeshComponent* StrokeStaticMeshComponent = dynamic_cast<const UStrokeStaticMeshComponent *>(ComponentPtr);
							if (MaterialInterface == StrokeStaticMeshComponent->SecondPassMaterial)
							{
								continue;
							}
							if (StrokeStaticMeshComponent->NeedSecondPass) {
								if (StrokeStaticMeshComponent->SecondPassMaterial == nullptr) {
									continue;
								}
								MeshBatch.MaterialRenderProxy = StrokeStaticMeshComponent->SecondPassMaterial->GetRenderProxy();
								MeshBatch.ReverseCulling = true;
								PDI->DrawMesh(MeshBatch, FLT_MAX);
							}
						}
					}
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE


