#pragma once

#include "Engine/Private/SkeletalRenderGPUSkin.h"

class FStrokeSkeletalMeshObjectGPUSkin : public FSkeletalMeshObjectGPUSkin
{
public:
	FStrokeSkeletalMeshObjectGPUSkin(USkinnedMeshComponent* InMeshComponent, FSkeletalMeshRenderData* InSkelMeshRenderData, ERHIFeatureLevel::Type InFeatureLevel):FSkeletalMeshObjectGPUSkin(InMeshComponent,InSkelMeshRenderData,InFeatureLevel){}
	
	FGPUSkinCacheEntry* GetGPUSkinCacheEntry(){return SkinCacheEntry;}
};

static int32 GForceRecomputeTangents = 0;
int32 GSkinCacheRecomputeTangents = 2;
static int32 GBlendUsingVertexColorForRecomputeTangents = 0;

class FGPUSkinCacheEntry
{
public:
	FGPUSkinCacheEntry(FGPUSkinCache* InSkinCache, FSkeletalMeshObjectGPUSkin* InGPUSkin, FGPUSkinCache::FRWBuffersAllocation* InPositionAllocation)
		: PositionAllocation(InPositionAllocation)
		, SkinCache(InSkinCache)
		, GPUSkin(InGPUSkin)
		, MorphBuffer(0)
		, LOD(InGPUSkin->GetLOD())
	{
		
		const TArray<FSkelMeshRenderSection>& Sections = InGPUSkin->GetRenderSections(LOD);
		DispatchData.AddDefaulted(Sections.Num());
		BatchElementsUserData.AddZeroed(Sections.Num());
		for (int32 Index = 0; Index < Sections.Num(); ++Index)
		{
			BatchElementsUserData[Index].Entry = this;
			BatchElementsUserData[Index].Section = Index;
		}

		UpdateSkinWeightBuffer();
	}

	~FGPUSkinCacheEntry()
	{
		check(!PositionAllocation);
	}

	struct FSectionDispatchData
	{
		FGPUSkinCache::FRWBufferTracker PositionTracker;

		FGPUBaseSkinVertexFactory* SourceVertexFactory = nullptr;
		FGPUSkinPassthroughVertexFactory* TargetVertexFactory = nullptr;

		// triangle index buffer (input for the RecomputeSkinTangents, might need special index buffer unique to position and normal, not considering UV/vertex color)
		FRHIShaderResourceView* IndexBuffer = nullptr;

		const FSkelMeshRenderSection* Section = nullptr;

		// for debugging / draw events, -1 if not set
		uint32 SectionIndex = -1;

		// 0:normal, 1:with morph target, 2:with APEX cloth (not yet implemented)
		uint16 SkinType = 0;

		// See EGPUSkinCacheDispatchFlags
		uint16 DispatchFlags = 0;

		//
		uint32 NumBoneInfluences = 0;

		// in floats (4 bytes)
		uint32 OutputStreamStart = 0;
		uint32 NumVertices = 0;

		// in vertices
		uint32 InputStreamStart = 0;
		uint32 NumTexCoords = 1;
		uint32 SelectedTexCoord = 0;

		FShaderResourceViewRHIRef TangentBufferSRV = nullptr;
		FShaderResourceViewRHIRef UVsBufferSRV = nullptr;
		FShaderResourceViewRHIRef ColorBufferSRV = nullptr;
		FShaderResourceViewRHIRef PositionBufferSRV = nullptr;
		FShaderResourceViewRHIRef ClothPositionsAndNormalsBuffer = nullptr;

		// skin weight input
		uint32 InputWeightStart = 0;

		// morph input
		uint32 MorphBufferOffset = 0;

        // cloth input
		uint32 ClothBufferOffset = 0;
        float ClothBlendWeight = 0.0f;

        FMatrix ClothLocalToWorld = FMatrix::Identity;
        FMatrix ClothWorldToLocal = FMatrix::Identity;

		// triangle index buffer (input for the RecomputeSkinTangents, might need special index buffer unique to position and normal, not considering UV/vertex color)
		uint32 IndexBufferOffsetValue = 0;
		uint32 NumTriangles = 0;

		FRWBuffer* TangentBuffer = nullptr;
		FRWBuffer* IntermediateTangentBuffer = nullptr;
		FRWBuffer* PositionBuffer = nullptr;
		FRWBuffer* PreviousPositionBuffer = nullptr;

        // Handle duplicates
        FShaderResourceViewRHIRef DuplicatedIndicesIndices = nullptr;
        FShaderResourceViewRHIRef DuplicatedIndices = nullptr;

		FSectionDispatchData() = default;

		inline FRWBuffer* GetPreviousPositionRWBuffer()
		{
			check(PreviousPositionBuffer);
			return PreviousPositionBuffer;
		}

		inline FRWBuffer* GetPositionRWBuffer()
		{
			check(PositionBuffer);
			return PositionBuffer;
		}

		inline FRHIShaderResourceView* GetPreSkinPositionSRV()
		{
			check(SourceVertexFactory);
			check(SourceVertexFactory->GetPositionsSRV());

			return SourceVertexFactory->GetPositionsSRV().GetReference();
		}

		inline FRWBuffer* GetTangentRWBuffer()
		{
			return TangentBuffer;
		}

		FRWBuffer* GetActiveTangentRWBuffer()
		{
			bool bUseIntermediateTangentBuffer = IndexBuffer && GBlendUsingVertexColorForRecomputeTangents > 0;

			if (bUseIntermediateTangentBuffer)
			{
				return IntermediateTangentBuffer;
			}
			else
			{
				return TangentBuffer;
			}
		}

		void UpdateVertexFactoryDeclaration()
		{
			TargetVertexFactory->UpdateVertexDeclaration(SourceVertexFactory, GetPositionRWBuffer(), GetPreSkinPositionSRV(), GetTangentRWBuffer());
		}
	};

	void UpdateVertexFactoryDeclaration(int32 Section)
	{
		DispatchData[Section].UpdateVertexFactoryDeclaration();
	}

	inline FCachedGeometry::Section GetCachedGeometry(int32 SectionIndex) const
	{
		FCachedGeometry::Section MeshSection;
		const FSkelMeshRenderSection& Section = *DispatchData[SectionIndex].Section;
		MeshSection.PositionBuffer	= DispatchData[SectionIndex].PositionBuffer->SRV;
		MeshSection.UVsBuffer		= DispatchData[SectionIndex].UVsBufferSRV;
		MeshSection.TotalVertexCount= DispatchData[SectionIndex].PositionBuffer->NumBytes / (sizeof(float)*3);
		MeshSection.NumPrimitives	= Section.NumTriangles;
		MeshSection.NumVertices		= Section.NumVertices;
		MeshSection.IndexBaseIndex	= Section.BaseIndex;
		MeshSection.VertexBaseIndex = Section.BaseVertexIndex;
		MeshSection.IndexBuffer		= nullptr;
		MeshSection.TotalIndexCount	= 0;
		MeshSection.LODIndex		= 0;
		MeshSection.SectionIndex	= SectionIndex;
		return MeshSection;
	}

	bool IsSectionValid(int32 Section) const
	{
		const FSectionDispatchData& SectionData = DispatchData[Section];
		return SectionData.SectionIndex == Section;
	}

	bool IsSourceFactoryValid(int32 Section, FGPUBaseSkinVertexFactory* SourceVertexFactory) const
	{
		const FSectionDispatchData& SectionData = DispatchData[Section];
		return SectionData.SourceVertexFactory == SourceVertexFactory;
	}

	bool IsValid(FSkeletalMeshObjectGPUSkin* InSkin) const
	{
		return GPUSkin == InSkin && GPUSkin->GetLOD() == LOD;
	}

	void UpdateSkinWeightBuffer()
	{
		FSkinWeightVertexBuffer* WeightBuffer = GPUSkin->GetSkinWeightVertexBuffer(LOD);
		bUse16BitBoneIndex = WeightBuffer->Use16BitBoneIndex();
		InputWeightIndexSize = WeightBuffer->GetBoneIndexByteSize();
		InputWeightStride = WeightBuffer->GetConstantInfluencesVertexStride();
		InputWeightStreamSRV = WeightBuffer->GetDataVertexBuffer()->GetSRV();
		InputWeightLookupStreamSRV = WeightBuffer->GetLookupVertexBuffer()->GetSRV();
				
		if (WeightBuffer->GetBoneInfluenceType() == GPUSkinBoneInfluenceType::DefaultBoneInfluence)
		{
			int32 MaxBoneInfluences = WeightBuffer->GetMaxBoneInfluences();
			BoneInfluenceType = MaxBoneInfluences > MAX_INFLUENCES_PER_STREAM ? 1 : 0;
		}
		else
		{
			BoneInfluenceType = 2;
		}
	}

	void SetupSection(int32 SectionIndex, FGPUSkinCache::FRWBuffersAllocation* InPositionAllocation, FSkelMeshRenderSection* Section, const FMorphVertexBuffer* MorphVertexBuffer, const FSkeletalMeshVertexClothBuffer* ClothVertexBuffer,
		uint32 NumVertices, uint32 InputStreamStart, FGPUBaseSkinVertexFactory* InSourceVertexFactory, FGPUSkinPassthroughVertexFactory* InTargetVertexFactory)
	{
		//UE_LOG(LogSkinCache, Warning, TEXT("*** SetupSection E %p Alloc %p Sec %d(%p) LOD %d"), this, InAllocation, SectionIndex, Section, LOD);
		FSectionDispatchData& Data = DispatchData[SectionIndex];
		check(!Data.PositionTracker.Allocation || Data.PositionTracker.Allocation == InPositionAllocation);

		Data.PositionTracker.Allocation = InPositionAllocation;

		Data.SectionIndex = SectionIndex;
		Data.Section = Section;

		check(GPUSkin->GetLOD() == LOD);
		FSkeletalMeshRenderData& SkelMeshRenderData = GPUSkin->GetSkeletalMeshRenderData();
		FSkeletalMeshLODRenderData& LodData = SkelMeshRenderData.LODRenderData[LOD];
		check(Data.SectionIndex == LodData.FindSectionIndex(*Section));

		Data.NumVertices = NumVertices;
		const bool bMorph = MorphVertexBuffer && MorphVertexBuffer->SectionIds.Contains(SectionIndex);
		if (bMorph)
		{
			// in bytes
			const uint32 MorphStride = sizeof(FMorphGPUSkinVertex);

			// see GPU code "check(MorphStride == sizeof(float) * 6);"
			check(MorphStride == sizeof(float) * 6);

			Data.MorphBufferOffset = Section->BaseVertexIndex;
		}
		if (ClothVertexBuffer && ClothVertexBuffer->GetClothIndexMapping().Num() > SectionIndex)
		{
			Data.ClothBufferOffset = (ClothVertexBuffer->GetClothIndexMapping()[SectionIndex] & 0xFFFFFFFF);
		}

		//INC_DWORD_STAT(STAT_GPUSkinCache_TotalNumChunks);

		// SkinType 0:normal, 1:with morph target, 2:with cloth
		Data.SkinType = ClothVertexBuffer ? 2 : (bMorph ? 1 : 0);
		Data.InputStreamStart = InputStreamStart;
		Data.OutputStreamStart = Section->BaseVertexIndex;

		Data.TangentBufferSRV = InSourceVertexFactory->GetTangentsSRV();
		Data.UVsBufferSRV = InSourceVertexFactory->GetTextureCoordinatesSRV();
		Data.ColorBufferSRV = InSourceVertexFactory->GetColorComponentsSRV();
		Data.NumTexCoords = InSourceVertexFactory->GetNumTexCoords();
		Data.PositionBufferSRV = InSourceVertexFactory->GetPositionsSRV();

		Data.NumBoneInfluences = InSourceVertexFactory->GetNumBoneInfluences();
		check(Data.TangentBufferSRV && Data.PositionBufferSRV);

		// weight buffer
		Data.InputWeightStart = (InputWeightStride * Section->BaseVertexIndex) / sizeof(float);
		Data.SourceVertexFactory = InSourceVertexFactory;
		Data.TargetVertexFactory = InTargetVertexFactory;

		InTargetVertexFactory->InvalidateStreams();

		int32 RecomputeTangentsMode = GForceRecomputeTangents > 0 ? 1 : GSkinCacheRecomputeTangents;
		if (RecomputeTangentsMode > 0)
		{
			if (Section->bRecomputeTangent || RecomputeTangentsMode == 1)
			{
				FRawStaticIndexBuffer16or32Interface* IndexBuffer = LodData.MultiSizeIndexContainer.GetIndexBuffer();
				Data.IndexBuffer = IndexBuffer->GetSRV();
				if (Data.IndexBuffer)
				{
					Data.NumTriangles = Section->NumTriangles;
					Data.IndexBufferOffsetValue = Section->BaseIndex;
				}
			}
		}
	}

#if RHI_RAYTRACING
	void GetRayTracingSegmentVertexBuffers(TArrayView<FRayTracingGeometrySegment> OutSegments) const
	{
		check(OutSegments.Num() == DispatchData.Num());

		for (int32 SectionIdx = 0; SectionIdx < DispatchData.Num(); SectionIdx++)
		{
			const FSectionDispatchData& SectionData = DispatchData[SectionIdx];
			FRayTracingGeometrySegment& Segment = OutSegments[SectionIdx];

			Segment.VertexBuffer = SectionData.PositionBuffer->Buffer;
			Segment.VertexBufferOffset = 0;

			check(SectionData.Section->NumTriangles == Segment.NumPrimitives);
		}
	}
#endif // RHI_RAYTRACING

protected:
public:
	FGPUSkinCache::FRWBuffersAllocation* PositionAllocation;
	FGPUSkinCache* SkinCache;
	TArray<FGPUSkinBatchElementUserData> BatchElementsUserData;
	TArray<FSectionDispatchData> DispatchData;
	FSkeletalMeshObjectGPUSkin* GPUSkin;
	int BoneInfluenceType;
	bool bUse16BitBoneIndex;
	uint32 InputWeightIndexSize;
	uint32 InputWeightStride;
	uint32 VertexOffsetUsage = 0;
	FShaderResourceViewRHIRef InputWeightStreamSRV;
	FShaderResourceViewRHIRef InputWeightLookupStreamSRV;
	FRHIShaderResourceView* PreSkinningVertexOffsetSRV = nullptr;
	FRHIShaderResourceView* PostSkinningVertexOffsetSRV = nullptr;
	FRHIShaderResourceView* MorphBuffer;
	FShaderResourceViewRHIRef ClothBuffer;
	int32 LOD;

	bool bMultipleClothSkinInfluences;

	friend class FGPUSkinCache;
	friend class FBaseGPUSkinCacheCS;
	friend class FBaseRecomputeTangentsPerTriangleShader;
};