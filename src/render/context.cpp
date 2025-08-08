#include "context.h"

#include "buffer.h"
#include "pipelinestate.h"
#include "rootsignature.h"
#include "device.h"
#include "commandlist.h"
#include "commandqueue.h"
#include "d3dconvert.h"

#define USE_PIX
#include "pix3.h"

namespace vkr::Render
{
	extern thread_local Context* g_CurrentContext = nullptr;
	extern thread_local Context* g_PrevContext = nullptr;

	Context::Context(ContextType type, const Ref<CommandQueue>& commandQueue)
		: m_CommandQueue(commandQueue)
		, m_CurrentD3DCommandList(nullptr)
		, m_CurrentD3DCommandList7(nullptr)
		, m_NumRecordedCommands(0)
		, m_Type(type)
	{
	}

	Context::~Context()
	{
	}

	void Context::Begin()
	{
		m_CommandList = GetDevice()->GetCommandListPool(m_Type)->GetCommandList();
		m_CurrentD3DCommandList = m_CommandList->GetD3DCommandList();
		m_CurrentD3DCommandList->QueryInterface(IID_PPV_ARGS(&m_CurrentD3DCommandList7));
		m_CommandList->Open();

		g_PrevContext = g_CurrentContext ? g_CurrentContext : nullptr;
		g_CurrentContext = this;
		m_NumRecordedCommands = 0;

		m_StateCache.m_TopologyDirty = true;
		m_StateCache.m_VertexBuffersDirty = true;
		m_StateCache.m_IndexBufferDirty = true;
		m_StateCache.m_RootSignatureDirty = true;
		m_StateCache.m_PipelineStateDirty = true;
		m_StateCache.m_RenderTargetsDirty = true;
		m_StateCache.m_LocalConstantsDirty.fill(true);
		m_StateCache.m_GlobalConstantsDirty.fill(true);
	}

	void Context::End()
	{
		// insert potential auto transitions/barriers
		if (m_CommandList)
		{
			m_CommandList->Close();
			if (m_NumRecordedCommands > 0)
			{
				m_CommandListsToSubmit.push_back(m_CommandList);
			}
			else
			{
				CommandListPool::PendingCommandLists pending;
				pending.m_CommandLists.push_back(m_CommandList);
				pending.m_Event = m_LastFlushEvent;
				GetDevice()->GetCommandListPool(m_Type)->ReturnCommandList(pending);
			}
		}

		if (m_CurrentD3DCommandList7)
			m_CurrentD3DCommandList7->Release();
		m_CurrentD3DCommandList7 = nullptr;
		m_CurrentD3DCommandList = nullptr;
		m_CommandList = nullptr;
		g_CurrentContext = g_PrevContext;
	}

	Fence Context::Flush()
	{
		if (!m_CommandListsToSubmit.empty())
		{
			for (uint32_t i = 0; i < m_FencesToWaitFor.size(); ++i)
			{
				m_CommandQueue->InsertWait(m_FencesToWaitFor[i]);
			}
			m_FencesToWaitFor.clear();

			m_LastFlushEvent = m_CommandQueue->Submit(m_CommandListsToSubmit.size(), m_CommandListsToSubmit.data());

			CommandListPool::PendingCommandLists pending;
			pending.m_CommandLists.insert(pending.m_CommandLists.end(), m_CommandListsToSubmit.begin(), m_CommandListsToSubmit.end());
			pending.m_Event = m_LastFlushEvent;
			GetDevice()->GetCommandListPool(m_Type)->ReturnCommandList(pending);

			m_CommandListsToSubmit.clear();
		}
		return m_LastFlushEvent;
	}

	void Context::ClearStateCache()
	{
		m_StateCache.Clear();
	}

	void Context::BeginMarker(const char* label, uint32_t color)
	{
		PIXBeginEvent(m_CurrentD3DCommandList, (0xff000000u | color), label);
	}

	void Context::EndMarker()
	{
		PIXEndEvent(m_CurrentD3DCommandList);
	}

	void Context::Dispatch(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ)
	{
		UpdateState();
		m_CurrentD3DCommandList->Dispatch(numGroupsX, numGroupsY, numGroupsZ);
		++m_NumRecordedCommands;
	}

	void Context::Dispatch(const Vector3u& Groups)
	{
		Dispatch(Groups.x, Groups.y, Groups.z);
	}

	void Context::DispatchThreads(uint32_t numThreadsX, uint32_t numThreadsY, uint32_t numThreadsZ)
	{
		assert(m_StateCache.m_PipelineState);
		const PipelineStateMetaData& metaData = m_StateCache.m_PipelineState->GetMetaData();
		assert(metaData.m_Type == PIPELINE_STATE_TYPE_COMPUTE);

		Vector3u threadGroups;
		threadGroups.x = (numThreadsX + metaData.Compute.m_NumThreads.x - 1) / metaData.Compute.m_NumThreads.x;
		threadGroups.y = (numThreadsY + metaData.Compute.m_NumThreads.y - 1) / metaData.Compute.m_NumThreads.y;
		threadGroups.z = (numThreadsZ + metaData.Compute.m_NumThreads.z - 1) / metaData.Compute.m_NumThreads.z;
		Dispatch(threadGroups);
	}

	void Context::DispatchThreads(PipelineState* pipelineState, const Vector3u& threads)
	{
		BindPipelineState(pipelineState);
		DispatchThreads(threads);
	}

	void Context::DispatchRays(PipelineState* pipelineState, uint32_t numThreadsX, uint32_t numThreadsY, uint32_t numThreadsZ)
	{
		assert(pipelineState->GetType() == PIPELINE_STATE_TYPE_RAYTRACING);
		BindPipelineState(pipelineState);
		UpdateState();
		D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = pipelineState->GetD3DDispatchRaysDesc();
		dispatchRaysDesc.Width = numThreadsX;
		dispatchRaysDesc.Height = numThreadsY;
		dispatchRaysDesc.Depth = numThreadsZ;
		m_CurrentD3DCommandList7->DispatchRays(&dispatchRaysDesc);
		++m_NumRecordedCommands;
	}

	void Context::DispatchRays(PipelineState* pipelineState, const Vector3u& numThreads)
	{
		DispatchRays(pipelineState, numThreads.x, numThreads.y, numThreads.z);
	}

	void Context::DispatchThreads(const Vector3u& threads)
	{
		DispatchThreads(threads.x, threads.y, threads.z);
	}

	void Context::DispatchThreads(PipelineState* pipelineState, uint32_t numThreadsX, uint32_t numThreadsY, uint32_t numThreadsZ)
	{
		BindPipelineState(pipelineState);
		DispatchThreads(numThreadsX, numThreadsY, numThreadsZ);
	}

	void Context::BindPipelineState(PipelineState* pipelineState)
	{
		m_StateCache.m_PipelineState = pipelineState;
		m_StateCache.m_PipelineStateDirty = true;

		RootSignature* rootSignature = pipelineState->GetRootSignature().get();
		if (m_StateCache.m_RootSignature != rootSignature)
		{
			// when root signature changes, perform a full cache flush?
			m_StateCache.m_RootSignature = rootSignature;
			m_StateCache.m_RootSignatureDirty = true;
			m_StateCache.m_LocalConstantsDirty.fill(true);
			m_StateCache.m_GlobalConstantsDirty.fill(true);
			m_StateCache.m_TopologyDirty = true;
			m_StateCache.m_VertexBuffersDirty = true;
			m_StateCache.m_IndexBufferDirty = true;
			m_StateCache.m_RootSignatureDirty = true;
			m_StateCache.m_PipelineStateDirty = true;
			m_StateCache.m_RenderTargetsDirty = true;
		}
	}

	void Context::BindLocalConstantBuffer(Buffer* buffer, uint64_t offset, uint32_t slot)
	{
		m_StateCache.m_LocalConstantBuffers[slot] = buffer;
		m_StateCache.m_LocalConstantBufferOffsets[slot] = offset;
		m_StateCache.m_LocalConstantsDirty[slot] = true;
	}

	void Context::BindLocalConstantBuffer(uint32_t byteSize, const void* data, uint32_t slot)
	{
		TempBuffer buf = GetDevice()->GetTempBuffer(TEMP_BUFFER_USAGE_CONSTANTS, byteSize, byteSize, data);
		m_StateCache.m_LocalConstantBuffers[slot] = buf.m_Buffer.get();
		m_StateCache.m_LocalConstantBufferOffsets[slot] = buf.m_Offset;
		m_StateCache.m_LocalConstantsDirty[slot] = true;
	}

	void Context::BindGlobalConstantBuffer(Buffer* buffer, uint64_t offset, GlobalConstantBufferSlot slot)
	{
		m_StateCache.m_GlobalConstantBuffers[slot] = buffer;
		m_StateCache.m_GlobalConstantBufferOffsets[slot] = offset;
		m_StateCache.m_GlobalConstantsDirty[slot] = true;
	}

	void Context::BindGlobalConstantBuffer(uint32_t byteSize, const void* data, GlobalConstantBufferSlot slot)
	{
		TempBuffer buf = GetDevice()->GetTempBuffer(TEMP_BUFFER_USAGE_CONSTANTS, byteSize, byteSize, data);
		m_StateCache.m_GlobalConstantBuffers[slot] = buf.m_Buffer.get();
		m_StateCache.m_GlobalConstantBufferOffsets[slot] = buf.m_Offset;
		m_StateCache.m_GlobalConstantsDirty[slot] = true;
	}

	void Context::TextureBarrier(uint32_t numBarriers, const TextureBarrierDesc* barrierDescs)
	{
		// TODO: defer barriers to group them better?

		std::vector<D3D12_TEXTURE_BARRIER> barriers;
		for (uint32_t i = 0; i < numBarriers; ++i)
		{
			const TextureBarrierDesc& barrierDesc = barrierDescs[i];
			ResourceStateTracking& stateTracking = barrierDesc.m_Texture->GetStateTracking();

			D3D12_TEXTURE_BARRIER barrier = {};
			barrier.AccessAfter = D3DConvertResourceStateAccess(barrierDesc.m_TargetAccess);
			barrier.AccessBefore = D3DConvertResourceStateAccess(stateTracking.m_CurrentAccess);
			barrier.SyncAfter = D3DConvertResourceStateSync(barrierDesc.m_TargetSync);
			barrier.SyncBefore = D3DConvertResourceStateSync(stateTracking.m_CurrentSync);
			barrier.LayoutAfter = D3DConvertResourceStateLayout(barrierDesc.m_TargetLayout);
			barrier.LayoutBefore = D3DConvertResourceStateLayout(stateTracking.m_CurrentLayout);
			barrier.pResource = barrierDesc.m_Texture->GetD3DResource();

			barrier.Subresources.IndexOrFirstMipLevel = 0xffffffff;
			barrier.Subresources.NumMipLevels = 0;

			barriers.push_back(barrier);
			stateTracking.m_CurrentAccess = barrierDesc.m_TargetAccess;
			stateTracking.m_CurrentLayout = barrierDesc.m_TargetLayout;
			stateTracking.m_CurrentSync = barrierDesc.m_TargetSync;
		}

		if (!barriers.empty())
		{
			D3D12_BARRIER_GROUP barrierGroup = {};
			barrierGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
			barrierGroup.pTextureBarriers = barriers.data();
			barrierGroup.NumBarriers = barriers.size();

			m_CurrentD3DCommandList7->Barrier(1, &barrierGroup);
			++m_NumRecordedCommands;
		}
	}

	void Context::TextureBarrier(const TextureBarrierDesc& barrierDesc)
	{
		TextureBarrier(1, &barrierDesc);
	}

	void Context::BufferBarrier(uint32_t numBarriers, const BufferBarrierDesc* barrierDescs)
	{
		// TODO: defer barriers to group them better?

		std::vector<D3D12_BUFFER_BARRIER> barriers;
		for (uint32_t i = 0; i < numBarriers; ++i)
		{
			const BufferBarrierDesc& barrierDesc = barrierDescs[i];
			ResourceStateTracking& stateTracking = barrierDesc.m_Buffer->GetStateTracking();

			D3D12_BUFFER_BARRIER barrier = {};
			barrier.AccessAfter = D3DConvertResourceStateAccess(barrierDesc.m_TargetAccess);
			barrier.AccessBefore = D3DConvertResourceStateAccess(stateTracking.m_CurrentAccess);
			barrier.SyncAfter = D3DConvertResourceStateSync(barrierDesc.m_TargetSync);
			barrier.SyncBefore = D3DConvertResourceStateSync(stateTracking.m_CurrentSync);
			barrier.pResource = barrierDesc.m_Buffer->GetD3DResource();
			barrier.Offset = 0;
			barrier.Size = barrierDesc.m_Buffer->GetDesc().ByteSize();

			barriers.push_back(barrier);
			stateTracking.m_CurrentAccess = barrierDesc.m_TargetAccess;
			stateTracking.m_CurrentSync = barrierDesc.m_TargetSync;
		}

		if (!barriers.empty())
		{
			D3D12_BARRIER_GROUP barrierGroup = {};
			barrierGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
			barrierGroup.pBufferBarriers = barriers.data();
			barrierGroup.NumBarriers = barriers.size();

			m_CurrentD3DCommandList7->Barrier(1, &barrierGroup);
			++m_NumRecordedCommands;
		}
	}

	void Context::BufferBarrier(const BufferBarrierDesc& barrierDesc)
	{
		BufferBarrier(1, &barrierDesc);
	}

	void Context::GlobalBarrier(uint32_t numBarriers, const GlobalBarrierDesc* barrierDescs)
	{
		// TODO: defer barriers to group them better?

		std::vector<D3D12_GLOBAL_BARRIER> barriers;
		for (uint32_t i = 0; i < numBarriers; ++i)
		{
			const GlobalBarrierDesc& barrierDesc = barrierDescs[i];

			D3D12_GLOBAL_BARRIER barrier = {};
			barrier.AccessAfter = D3DConvertResourceStateAccess(barrierDesc.m_TargetAccess);
			barrier.AccessBefore = D3DConvertResourceStateAccess(barrierDesc.m_SourceAccess);
			barrier.SyncAfter = D3DConvertResourceStateSync(barrierDesc.m_TargetSync);
			barrier.SyncBefore = D3DConvertResourceStateSync(barrierDesc.m_SourceSync);

			barriers.push_back(barrier);
		}

		if (!barriers.empty())
		{
			D3D12_BARRIER_GROUP barrierGroup = {};
			barrierGroup.Type = D3D12_BARRIER_TYPE_GLOBAL;
			barrierGroup.pGlobalBarriers = barriers.data();
			barrierGroup.NumBarriers = barriers.size();

			m_CurrentD3DCommandList7->Barrier(1, &barrierGroup);
			++m_NumRecordedCommands;
		}
	}

	void Context::GlobalBarrier(const GlobalBarrierDesc& barrierDesc)
	{
		GlobalBarrier(1, &barrierDesc);
	}

	void Context::UpdateState()
	{
		const PipelineStateType rootSignatureType = m_StateCache.m_RootSignature ? m_StateCache.m_RootSignature->GetType() : PIPELINE_STATE_TYPE_UNKNOWN;
		const bool useGraphicsPipeline = rootSignatureType == PIPELINE_STATE_TYPE_DEFAULT;
		if (m_StateCache.m_RootSignatureDirty)
		{
			if (m_StateCache.m_RootSignature)
			{
				ID3D12DescriptorHeap* descriptorHeaps[2] =
				{
					GetDevice()->GetDescriptorHeap(DESCRIPTOR_HEAP_TYPE_SHADER_RESOURCE)->GetD3DDescriptorHeap(),
					GetDevice()->GetDescriptorHeap(DESCRIPTOR_HEAP_TYPE_SAMPLER)->GetD3DDescriptorHeap()
				};
				m_CurrentD3DCommandList->SetDescriptorHeaps(2, descriptorHeaps);

				if (useGraphicsPipeline)
				{
					m_CurrentD3DCommandList->SetGraphicsRootSignature(m_StateCache.m_RootSignature->GetD3DRootSignature());
				}
				else
				{
					m_CurrentD3DCommandList->SetComputeRootSignature(m_StateCache.m_RootSignature->GetD3DRootSignature());
				}
			}
			m_StateCache.m_RootSignatureDirty = false;
		}

		if (m_StateCache.m_PipelineStateDirty)
		{
			if (m_StateCache.m_PipelineState)
			{
				if (m_StateCache.m_PipelineState->GetType() == PIPELINE_STATE_TYPE_RAYTRACING)
				{
					m_CurrentD3DCommandList7->SetPipelineState1(m_StateCache.m_PipelineState->GetD3DStateObject());
				}
				else
				{
					m_CurrentD3DCommandList->SetPipelineState(m_StateCache.m_PipelineState->GetD3DPipelineState());
				}
			}
			else
			{
				m_CurrentD3DCommandList->SetPipelineState(nullptr);
			}
			m_StateCache.m_PipelineStateDirty = false;
		}

		if (m_StateCache.m_VertexBuffersDirty && useGraphicsPipeline)
		{
			std::vector<D3D12_VERTEX_BUFFER_VIEW> bufferViews;
			for (uint32_t i = 0; i < m_StateCache.m_VertexBuffers.size(); ++i)
			{
				const Buffer* buffer = m_StateCache.m_VertexBuffers[i];
				if (buffer)
				{
					const uint64_t offset = m_StateCache.m_VertexBufferOffsets.empty() ? 0 : m_StateCache.m_VertexBufferOffsets[i];
					const uint64_t size = m_StateCache.m_VertexBufferSizes.empty() ? (buffer->GetDesc().m_ElementSize * buffer->GetDesc().m_ElementCount) : m_StateCache.m_VertexBufferSizes[i];
					const uint32_t stride = m_StateCache.m_VertexBufferStrides.empty() ? buffer->GetDesc().m_ElementSize : m_StateCache.m_VertexBufferStrides[i];

					D3D12_VERTEX_BUFFER_VIEW view;
					view.BufferLocation = buffer->GetD3DResource()->GetGPUVirtualAddress() + offset;
					view.SizeInBytes = size;
					view.StrideInBytes = stride;
					bufferViews.push_back(view);
				}
			}
			if (!bufferViews.empty())
			{
				m_CurrentD3DCommandList->IASetVertexBuffers(0, bufferViews.size(), bufferViews.data());
			}
			else
			{
				m_CurrentD3DCommandList->IASetVertexBuffers(0, 0, nullptr);
			}
			m_StateCache.m_VertexBuffersDirty = false;
		}

		if (m_StateCache.m_IndexBufferDirty && useGraphicsPipeline)
		{
			const Buffer* buffer = m_StateCache.m_IndexBuffer;
			if (buffer)
			{
				const uint64_t offset = m_StateCache.m_IndexBufferOffset;
				const uint64_t size = m_StateCache.m_IndexBufferSize == 0 ? (buffer->GetDesc().m_ElementSize * buffer->GetDesc().m_ElementCount) : m_StateCache.m_IndexBufferSize;
				const Format format = m_StateCache.m_IndexBufferFormat != FORMAT_UNKNOWN ? m_StateCache.m_IndexBufferFormat : buffer->GetDesc().m_Format;

				D3D12_INDEX_BUFFER_VIEW view;
				view.BufferLocation = buffer->GetD3DResource()->GetGPUVirtualAddress() + offset;
				view.SizeInBytes = size;
				view.Format = D3DConvertFormat(format);
				m_CurrentD3DCommandList->IASetIndexBuffer(&view);
			}
			else
			{
				m_CurrentD3DCommandList->IASetIndexBuffer(nullptr);
			}
			m_StateCache.m_IndexBufferDirty = false;
		}

		if (m_StateCache.m_TopologyDirty && useGraphicsPipeline)
		{
			m_CurrentD3DCommandList->IASetPrimitiveTopology(D3DConvertPrimitiveTopology(m_StateCache.m_Topology));
			m_StateCache.m_TopologyDirty = false;
		}

		if (m_StateCache.m_RootSignature)
		{
			const uint32_t localConstantsParamStartIndex = m_StateCache.m_RootSignature->GetLocalConstantBufferParameterStart();
			const uint32_t numLocalConstantsToLookFor = m_StateCache.m_RootSignature->GetNumLocalConstantBuffers();
			for (int i = 0; i < numLocalConstantsToLookFor; i++)
			{
				if (m_StateCache.m_LocalConstantsDirty[i])
				{
					const Buffer* buffer = m_StateCache.m_LocalConstantBuffers[i];
					if (buffer)
					{
						const uint64_t offset = m_StateCache.m_LocalConstantBufferOffsets[i];
						const D3D12_GPU_VIRTUAL_ADDRESS addr = buffer->GetD3DResource()->GetGPUVirtualAddress() + offset;
						if (useGraphicsPipeline)
						{
							m_CurrentD3DCommandList->SetGraphicsRootConstantBufferView(localConstantsParamStartIndex + i, addr);
						}
						else
						{
							m_CurrentD3DCommandList->SetComputeRootConstantBufferView(localConstantsParamStartIndex + i, addr);
						}
					}

					m_StateCache.m_LocalConstantsDirty[i] = false;
				}
			}

			const uint32_t globalConstantsParamStartIndex = m_StateCache.m_RootSignature->GetGlobalConstantBufferParameterStart();
			for (int i = 0; i < m_StateCache.m_GlobalConstantBuffers.size(); i++)
			{
				if (m_StateCache.m_GlobalConstantsDirty[i])
				{
					const Buffer* buffer = m_StateCache.m_GlobalConstantBuffers[i];
					if (buffer)
					{
						const uint64_t offset = m_StateCache.m_GlobalConstantBufferOffsets[i];
						const D3D12_GPU_VIRTUAL_ADDRESS addr = buffer->GetD3DResource()->GetGPUVirtualAddress() + offset;
						if (useGraphicsPipeline)
						{
							m_CurrentD3DCommandList->SetGraphicsRootConstantBufferView(globalConstantsParamStartIndex + i, addr);
						}
						else
						{
							m_CurrentD3DCommandList->SetComputeRootConstantBufferView(globalConstantsParamStartIndex + i, addr);
						}
					}
					m_StateCache.m_GlobalConstantsDirty[i] = false;
				}
			}
		}

		if (m_StateCache.m_RenderTargetsDirty && useGraphicsPipeline)
		{
			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs;
			rtvs.reserve(MAX_NUM_RENDER_TARGETS);
			for (const RenderTargetView* rtv : m_StateCache.m_RenderTargets)
			{
				if (rtv)
				{
					rtvs.push_back(rtv->GetHandle());
				}
			}

			const D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandles = rtvs.size() ? rtvs.data() : nullptr;
			const D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle = m_StateCache.m_DepthStencil ? &m_StateCache.m_DepthStencil->GetHandle() : nullptr;

			m_CurrentD3DCommandList->OMSetRenderTargets(rtvs.size(), rtvHandles, false, dsvHandle);

			m_StateCache.m_RenderTargetsDirty = false;
		}
	}

	void Context::ClearRenderTargets(uint32_t numRenderTargets, RenderTargetView** renderTargetViews, const Vector4f* clearValues)
	{
		static constexpr float DefaultClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		for (uint32_t i = 0; i < numRenderTargets; ++i)
		{
			m_CurrentD3DCommandList->ClearRenderTargetView(renderTargetViews[i]->GetHandle(), clearValues ? &clearValues[i].x : DefaultClearColor, 0, nullptr);
			++m_NumRecordedCommands;
		}
	}

	void Context::ClearRenderTarget(RenderTargetView* renderTargetView, const Vector4f& clearValue)
	{
		ClearRenderTargets(1, &renderTargetView, &clearValue);
	}

	void Context::ClearDepthStencil(DepthStencilView* dsv, float clearValue)
	{
		m_CurrentD3DCommandList->ClearDepthStencilView(dsv->GetHandle(), D3D12_CLEAR_FLAG_DEPTH, clearValue, 0, 0, nullptr);
		++m_NumRecordedCommands;
	}

	Ref<Buffer> Context::BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructureBuildDesc& desc)
	{
		Device* device = GetDevice();

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;
		if (desc.m_Type == RaytracingAccelerationStructureBuildDesc::Type::TopLevel)
		{
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs = buildDesc.Inputs;
			inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
			inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
			inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;

			if (!desc.m_InstanceDescs.empty())
			{
				std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
				instanceDescs.reserve(desc.m_InstanceDescs.size());
				for (uint32_t i = 0; i < desc.m_InstanceDescs.size(); ++i)
				{
					const RaytracingInstanceDesc& rtInstanceDesc = desc.m_InstanceDescs[i];
					D3D12_RAYTRACING_INSTANCE_DESC desc = {};
					desc.AccelerationStructure = rtInstanceDesc.m_BLAS->GetD3DResource()->GetGPUVirtualAddress();
					desc.InstanceID = rtInstanceDesc.m_InstanceId;
					desc.InstanceMask = 0xff;
					desc.InstanceContributionToHitGroupIndex = rtInstanceDesc.m_HitGroupIndex;
					desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE;
					// TODO: the other instance desc params

					for (uint32_t row = 0; row < 3; ++row)
					{
						for (uint32_t col = 0; col < 4; ++col)
						{
							desc.Transform[row][col] = rtInstanceDesc.m_Transform.At(col, row);
						}
					}

					instanceDescs.push_back(desc);
				}

				const uint32_t bufferSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceDescs.size();
				TempBuffer instanceDescsBuffer = device->GetTempBuffer(Render::TEMP_BUFFER_USAGE_STAGING, bufferSize, bufferSize, instanceDescs.data());
				inputs.InstanceDescs = instanceDescsBuffer.m_Buffer->GetD3DResource()->GetGPUVirtualAddress() + instanceDescsBuffer.m_Offset;
				inputs.NumDescs = instanceDescs.size();
			}
		}
		else if (desc.m_Type == RaytracingAccelerationStructureBuildDesc::Type::BottomLevel)
		{
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs = buildDesc.Inputs;
			inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
			inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
			inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

			if (!desc.m_GeometryDescs.empty())
			{
				geometryDescs.reserve(desc.m_GeometryDescs.size());
				for (uint32_t i = 0; i < desc.m_GeometryDescs.size(); ++i)
				{
					const RaytracingGeometryDesc& rtGeometryDesc = desc.m_GeometryDescs[i];
					D3D12_RAYTRACING_GEOMETRY_DESC desc = {};
					desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;

					const BufferDesc& indexBufferDesc = rtGeometryDesc.m_IndexBuffer->GetDesc();
					desc.Triangles.IndexBuffer = rtGeometryDesc.m_IndexBuffer->GetD3DResource()->GetGPUVirtualAddress();
					desc.Triangles.IndexFormat = D3DConvertFormat(indexBufferDesc.m_Format);
					desc.Triangles.IndexCount = indexBufferDesc.m_ElementCount;

					const BufferDesc& vertexBufferDesc = rtGeometryDesc.m_VertexBuffer->GetDesc();
					desc.Triangles.VertexBuffer.StartAddress = rtGeometryDesc.m_VertexBuffer->GetD3DResource()->GetGPUVirtualAddress();
					desc.Triangles.VertexBuffer.StrideInBytes = vertexBufferDesc.m_ElementSize;
					desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
					desc.Triangles.VertexCount = vertexBufferDesc.m_ElementCount;

					geometryDescs.push_back(desc);
				}

				inputs.pGeometryDescs = geometryDescs.data();
				inputs.NumDescs = geometryDescs.size();
			}
		}

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
		device->GetD3DDevice10()->GetRaytracingAccelerationStructurePrebuildInfo(&buildDesc.Inputs, &prebuildInfo);

		TempBuffer scratchBuffer = device->GetTempBuffer(TEMP_BUFFER_USAGE_RAYTRACING_ACCELERATION_STRUCTURE, prebuildInfo.ScratchDataSizeInBytes);
		buildDesc.ScratchAccelerationStructureData = scratchBuffer.m_Buffer->GetD3DResource()->GetGPUVirtualAddress() + scratchBuffer.m_Offset;

		BufferDesc outBufferDesc = {};
		outBufferDesc.m_ElementSize = 1;
		outBufferDesc.m_ElementCount = prebuildInfo.ResultDataMaxSizeInBytes;
		outBufferDesc.m_IsRaytracingAccelerationStructure = true;

		Ref<Buffer> outBuffer = device->CreateBuffer(outBufferDesc);
		assert(outBuffer);
		buildDesc.DestAccelerationStructureData = outBuffer->GetD3DResource()->GetGPUVirtualAddress();

		BufferBarrierDesc bufferBarrier = {};
		bufferBarrier.m_Buffer = outBuffer.get();
		bufferBarrier.m_TargetAccess = RESOURCE_STATE_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;
		bufferBarrier.m_TargetSync = RESOURCE_STATE_SYNC_ALL;
		BufferBarrier(bufferBarrier);

		m_CurrentD3DCommandList7->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
		++m_NumRecordedCommands;

		bufferBarrier.m_TargetAccess = RESOURCE_STATE_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ;
		bufferBarrier.m_TargetSync = RESOURCE_STATE_SYNC_ALL;
		BufferBarrier(bufferBarrier);

		return outBuffer;
	}

	ContextType Context::GetType() const
	{
		return m_Type;
	}

	CommandList* Context::GetCommandList() const
	{
		return m_CommandList.get();
	}

	const Fence& Context::GetLastFence() const
	{
		return m_LastFlushEvent;
	}

	Context* Context::GetCurrentContext()
	{
		return g_CurrentContext;
	}

	void Context::RenderStateCache::Clear()
	{
		m_Topology = PRIMITIVE_TOPOLOGY_UNDEFINED;

		m_VertexBuffers.clear();
		m_VertexBufferOffsets.clear();
		m_VertexBufferSizes.clear();
		m_VertexBufferStrides.clear();

		m_IndexBuffer = nullptr;
		m_IndexBufferOffset = 0;
		m_IndexBufferSize = 0;
		m_IndexBufferFormat = FORMAT_UNKNOWN;

		m_RootSignature = nullptr;
		m_PipelineState = nullptr;

		m_LocalConstantBuffers.fill(nullptr);
		m_LocalConstantBufferOffsets.fill(0);

		m_RenderTargets.fill(nullptr);
		m_DepthStencil = nullptr;

		m_RootSignatureDirty = false;
		m_PipelineStateDirty = false;
		m_LocalConstantsDirty.fill(false);
		m_VertexBuffersDirty = false;
		m_IndexBufferDirty = false;
		m_TopologyDirty = false;
		m_RenderTargetsDirty = false;
	}

	void Context::BindRenderTargets(uint32_t numRenderTargets, RenderTargetView** renderTargetViews)
	{
		m_StateCache.m_RenderTargets.fill(nullptr);
		for (uint32_t i = 0; i < numRenderTargets; ++i)
		{
			m_StateCache.m_RenderTargets[i] = renderTargetViews[i];
		}
		m_StateCache.m_RenderTargetsDirty = true;
	}

	void Context::BindRenderTarget(RenderTargetView* renderTargetView)
	{
		BindRenderTargets(1, &renderTargetView);
	}

	void Context::BindDepthStencil(DepthStencilView* depthStencilView)
	{
		m_StateCache.m_DepthStencil = depthStencilView;
		m_StateCache.m_RenderTargetsDirty = true;
	}

	void Context::BindVertexBuffers(uint32_t numVertexBuffers, Buffer** buffers, const uint64_t* offsets, const uint32_t* sizes, const uint32_t* strides)
	{
		m_StateCache.m_VertexBuffers = std::vector<Buffer*>(buffers, buffers + numVertexBuffers);
		m_StateCache.m_VertexBufferOffsets.clear();
		m_StateCache.m_VertexBufferSizes.clear();
		m_StateCache.m_VertexBufferStrides.clear();
		if (offsets)
		{
			m_StateCache.m_VertexBufferOffsets = std::vector<uint64_t>(offsets, offsets + numVertexBuffers);
		}
		if (sizes)
		{
			m_StateCache.m_VertexBufferSizes = std::vector<uint32_t>(sizes, sizes + numVertexBuffers);
		}
		if (strides)
		{
			m_StateCache.m_VertexBufferStrides = std::vector<uint32_t>(strides, strides + numVertexBuffers);
		}
		m_StateCache.m_VertexBuffersDirty = true;
	}

	void Context::BindVertexBuffer(Buffer* vertexBuffer, const uint64_t offset, const uint32_t size, const uint32_t stride)
	{
		const uint32_t actualSize = size ? size : vertexBuffer->GetDesc().ByteSize();
		const uint32_t actualStride = stride ? stride : vertexBuffer->GetDesc().m_ElementSize;
		BindVertexBuffers(1, &vertexBuffer, &offset, &actualSize, &actualStride);
	}

	void Context::BindIndexBuffer(Buffer* indexBuffer, const uint64_t offset, const uint32_t size, const Format format)
	{
		m_StateCache.m_IndexBuffer = indexBuffer;
		m_StateCache.m_IndexBufferOffset = offset;
		m_StateCache.m_IndexBufferSize = size;
		m_StateCache.m_IndexBufferFormat = format;
		m_StateCache.m_IndexBufferDirty = true;
	}

	void Context::Draw(uint32_t vertexCount, uint32_t startVertex)
	{
		DrawInstanced(vertexCount, 1, startVertex, 0);
	}

	void Context::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance)
	{
		UpdateState();
		m_CurrentD3DCommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
		++m_NumRecordedCommands;
	}

	void Context::DrawIndexed(uint32_t indexCount, uint32_t startIndex, uint32_t startVertex)
	{
		DrawIndexedInstanced(indexCount, 1, startIndex, startVertex, 0);
	}

	void Context::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, uint32_t startVertex, uint32_t startInstance)
	{
		UpdateState();
		m_CurrentD3DCommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, startVertex, startInstance);
		++m_NumRecordedCommands;
	}

	void Context::SetPrimitiveTopology(PrimitiveTopology topologyType)
	{
		m_StateCache.m_Topology = topologyType;
		m_StateCache.m_TopologyDirty = true;
	}

	void Context::SetViewport(uint32_t offsetX, uint32_t offsetY, uint32_t width, uint32_t height, float depthMin /*= 0.0f*/, float depthMax /*= 1.0f*/)
	{
		D3D12_VIEWPORT vp;
		vp.TopLeftX = offsetX;
		vp.TopLeftY = offsetY;
		vp.Width = width;
		vp.Height = height;
		vp.MinDepth = depthMin;
		vp.MaxDepth = depthMax;
		m_CurrentD3DCommandList->RSSetViewports(1, &vp);
		++m_NumRecordedCommands;
	}

	void Context::SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
	{
		D3D12_RECT rect;
		rect.left = left;
		rect.top = top;
		rect.right = right;
		rect.bottom = bottom;
		m_CurrentD3DCommandList->RSSetScissorRects(1, &rect);
		++m_NumRecordedCommands;
	}

	void Context::CopyResource(Buffer* dst, Buffer* src)
	{
		// validate that resources have the same layout
		m_CurrentD3DCommandList->CopyResource(dst->GetD3DResource(), src->GetD3DResource());
		++m_NumRecordedCommands;
	}

	void Context::CopyResource(Texture* dst, Texture* src)
	{
		// validate that resources have the same layout
		m_CurrentD3DCommandList->CopyResource(dst->GetD3DResource(), src->GetD3DResource());
		++m_NumRecordedCommands;
	}

	void Context::CopyBuffer(Buffer* dst, uint64_t dstOffset, Buffer* src, uint64_t srcOffset, uint32_t size)
	{
		m_CurrentD3DCommandList->CopyBufferRegion(dst->GetD3DResource(), dstOffset, src->GetD3DResource(), srcOffset, size);
		++m_NumRecordedCommands;
	}

	void Context::CopyTexture(Texture* dst, Texture* src)
	{
		D3D12_TEXTURE_COPY_LOCATION dstLocation;
		dstLocation.pResource = dst->GetD3DResource();
		dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dstLocation.SubresourceIndex = 0;

		D3D12_TEXTURE_COPY_LOCATION srcLocation;
		srcLocation.pResource = src->GetD3DResource();
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		srcLocation.SubresourceIndex = 0;
		m_CurrentD3DCommandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
		++m_NumRecordedCommands;
	}

	void Context::InsertWait(const Fence& fence)
	{
		m_FencesToWaitFor.push_back(fence);
	}

}