#pragma once
#include "light.h"
#include "render/buffer.h"
#include "render/device.h"
#include "render/renderstates.h"
#include "materialdatabuffer.h"

namespace vkr::Render
{
	class PipelineState;
	class Buffer;
	class BufferView;
}

namespace vkr::Graphics
{
	class Mesh;
	class MaterialInstance;
	class PipelineState;

	struct CameraData
	{
		Mat43 CameraWorldMatrix;
		Mat44 ViewMatrix;
		Mat44 ProjectionMatrix;
		Mat44 InvViewMatrix;
		Mat44 InvProjectionMatrix;
		Mat44 ViewProjectionMatrix;
		Mat44 InvViewProjectionMatrix;
		//Prev frame
		Mat44 PrevCameraWorldMatrix;
		Mat44 PrevViewMatrix;
		Mat44 PrevViewProjectionMatrix;
		//Unjittered
		Mat44 ProjectionMatrixUnjittered;
		Mat44 InvProjectionMatrixUnjittered;
		Mat44 ViewProjectionMatrixUnjittered;
		Mat44 InvViewProjectionMatrixUnjittered;
		//Unjittered prev frame
		Mat44 PrevProjectionMatrixUnjittered;
		Mat44 PrevViewProjectionMatrixUnjittered;

		Vector2f CurrentJitter;
		Vector2f PrevJitter;

		float AspectRatio;
		float Near;
		float Far;
		float FOVDegrees;
	};

	struct InstanceData
	{
		Mat44 m_Transform;
		Mat44 m_PrevTransform;
		uint32_t m_MaterialID;
		uint32_t m_IndexBufferDescriptorIndex;
		uint32_t m_IndexStride;
		uint32_t m_VertexBufferDescriptorIndex;
		uint32_t m_VertexStride;
		uint32_t m_VertexPositionByteOffset;
		uint32_t m_VertexNormalByteOffset;
		uint32_t m_VertexTangentByteOffset;
		uint32_t m_VertexUVByteOffset;
		uint32_t m_Pad0[3];
	};

	struct RenderObject
	{
		Ref<Render::Buffer> m_VB;
		Ref<Render::Buffer> m_IB;
		Render::PrimitiveTopology m_Topology;
		Render::VertexLayout m_VertexLayout;
		MaterialInstance* m_Material;
		uint32_t m_InstanceDataIndex;
		float m_DistanceToCamera;

		// State-sort operator material->mesh->distance
		bool operator<(const RenderObject& other) const
		{
			if (m_Material != other.m_Material)
				return m_Material < other.m_Material;

			if (m_VB != other.m_VB)
				return m_VB.get() < other.m_VB.get();

			if (m_IB != other.m_IB)
				return m_IB.get() < other.m_IB.get();

			return m_DistanceToCamera < other.m_DistanceToCamera;
		}
	};

	struct RenderBatch
	{
		Ref<Render::Buffer> m_VB = nullptr;
		Ref<Render::Buffer> m_IB = nullptr;
		Render::PrimitiveTopology m_Topology;
		Ref<Render::PipelineState> m_PSO = nullptr;
		size_t m_StartOffset = 0;
		size_t m_Count = 0;
	};

	struct MeshPassData
	{
		std::vector<RenderBatch> m_InstanceBatches;
	};

	//This structure strictly only holds physical parameters of the sky, not configuration parameters
	struct alignas(16) AtmosphereData
	{
		float MultiScatteringFactor;
		// The distance between the planet center and the bottom of the atmosphere.
		float BottomRadiusKm;
		// The distance between the ground and the top of the atmosphere.
		float TopRadiusKm;
		float RayleighDensityExpScale;
		Vector3f RayleighScattering;
		uint32_t _pad0;
		Vector3f MieScattering;
		float MieDensityExpScale;
		Vector3f MieExtinction;
		float MiePhaseG;
		Vector3f MieAbsorption;
		float AbsorptionDensity0LayerWidth;
		float AbsorptionDensity0ConstantTerm;
		float AbsorptionDensity0LinearTerm;
		float AbsorptionDensity1ConstantTerm;
		float AbsorptionDensity1LinearTerm;
		Vector3f AbsorptionExtinction;
		uint32_t _pad1;
		// The average albedo of the ground.
		Vector3f GroundAlbedo;
		uint32_t _pad2;
	};

	//This struct contains other data needed for the sky render, that are not strictly physical parameters.
	struct SkyData
	{
		Mat44 SkyViewLutReferential;
		Vector4f SkyViewLutSizeAndInvSize;
		Vector4f SkyPlanetTranslatedWorldCenterAndViewHeight;
		float FogShowFlagFactor;
		float AerialPerspectiveStartDepthKm;
		float AerialPerspectiveVolumeDepthKm;
		float AerialPerspectiveLutDepthResolution;
		Vector4f AerialPerspectiveLutSizeAndInvSize;
	};


	struct ViewRenderData
	{
		void Clear();

		Vector2u m_RenderSize;
		Vector2u m_OutputSize;

		CameraData m_CameraData;
		uint32_t m_FrameIndex;
		float m_DeltaTime;
		float m_ElapsedTime;

		Ref<Render::BufferView> m_RaytracingTLAS;
		std::vector<Render::RaytracingInstanceDesc> m_RaytracingInstances;

		std::vector<RenderObject> m_VisibleMeshes;
		std::vector<LocalLight> m_VisibleLights;
		uint32_t m_NumDirectionalLights;
		DirectionalLight m_DirectionalLights[2];
		MeshPassData m_DepthPassData;
		MeshPassData m_ForwardPassData;

		std::vector<uint8_t> m_InstanceData; //We might want to take this outside of the view prepare , and do it at scene update time
		Ref<Render::BufferView> m_InstanceDataBufferView;

		//Contains the set of indices into the instance data buffer to retrieve the instance data for every instance to render in the batches
		std::vector<uint32_t> m_InstanceDataOffsetBuffer;
		Ref<Render::BufferView> m_InstanceDataOffsetBufferView;

		MaterialDataBuffer m_MaterialDataBuffer;

		Ref<Render::PipelineState> m_TraceRaysPipelineState;

		Render::TempBuffer m_PerSceneConstantBuffer;
		
		bool m_UpdateSkyLut = true;
		Ref<Render::RenderTaskEvent> m_UpdateSkyLutEvent;

		Ref<Render::RenderTaskEvent> m_TerrainUpdateEvent;

		AtmosphereData m_AtmosphereData;
		SkyData m_SkyData;

		Mesh* m_TerrainMesh = nullptr; //Terrain mesh object to be used for this frame and that needs to be updated

		uint32_t m_TotalInstanceCount = 0;
	};
}