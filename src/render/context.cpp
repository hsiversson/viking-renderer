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
	thread_local Context* Context::g_CurrentContext = nullptr;

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
		g_CurrentContext = this;
		m_NumRecordedCommands = 0;
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
		CurrentState = {};
		g_CurrentContext = nullptr;
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

	void Context::BeginMarker(const char* label, uint32_t color)
	{
		PIXBeginEvent(m_CurrentD3DCommandList, (0xff000000u | color), label);
	}

	void Context::EndMarker()
	{
		PIXEndEvent(m_CurrentD3DCommandList);
	}

	void Context::Dispatch(const Vector3u& Groups)
	{
		UpdateState();
		m_CurrentD3DCommandList->Dispatch(Groups.x, Groups.y, Groups.z);
		++m_NumRecordedCommands;
	}

	void Context::DispatchThreads(Ref<PipelineState> pipelineState, const Vector3u& threads)
	{
		BindPSO(pipelineState);
		DispatchThreads(threads);
	}

	void Context::DispatchThreads(const Vector3u& threads)
	{
		assert(NewState.m_PipelineState);
		const PipelineStateMetaData& metaData = NewState.m_PipelineState->GetMetaData();
		assert(metaData.m_Type == PIPELINE_STATE_TYPE_COMPUTE);

		Vector3u threadGroups;
		threadGroups.x = (threads.x + metaData.Compute.m_NumThreads.x - 1) / metaData.Compute.m_NumThreads.x;
		threadGroups.y = (threads.y + metaData.Compute.m_NumThreads.y - 1) / metaData.Compute.m_NumThreads.y;
		threadGroups.z = (threads.z + metaData.Compute.m_NumThreads.z - 1) / metaData.Compute.m_NumThreads.z;
		Dispatch(threadGroups);
	}

	void Context::BindPSO(Ref<PipelineState> pipelineState)
	{
		NewState.m_PipelineState = pipelineState;
		NewState.m_RootSignature = pipelineState->GetRootSignature().get();
		m_StateUpdate = true;
	}

	void Context::BindRootConstantBuffers(Ref<Buffer>* buffers, size_t numBuffers, uint64_t* offsets)
	{
		for (uint32_t i = 0; i < numBuffers; ++i)
		{
			NewState.m_RootCB[i] = buffers[i];
			if (offsets)
			{
				NewState.m_RootCBOffsets[i] = offsets[i];
			}
			else
			{
				NewState.m_RootCBOffsets[i] = 0;
			}
		}
		m_StateUpdate = true;
	}

	void Context::BindRootConstantBuffer(uint32_t byteSize, const void* data, uint32_t slot)
	{
		TempBuffer buf = GetDevice()->GetTempBuffer(TEMP_BUFFER_USAGE_CONSTANTS, byteSize, byteSize, data);
		NewState.m_RootCB[slot] = buf.m_Buffer;
		NewState.m_RootCBOffsets[slot] = buf.m_Offset;
		m_StateUpdate = true;
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
		if (m_StateUpdate)
		{
			if (CurrentState.m_PipelineState != NewState.m_PipelineState)
			{
				m_CurrentD3DCommandList->SetPipelineState(NewState.m_PipelineState->GetD3DPipelineState());
			}
			if (CurrentState.m_RootSignature != NewState.m_RootSignature)
			{
				ID3D12DescriptorHeap* descriptorHeaps[2] = 
				{ 
					GetDevice()->GetDescriptorHeap(DESCRIPTOR_HEAP_TYPE_SHADER_RESOURCE)->GetD3DDescriptorHeap(),
					GetDevice()->GetDescriptorHeap(DESCRIPTOR_HEAP_TYPE_SAMPLER)->GetD3DDescriptorHeap()
				};
				m_CurrentD3DCommandList->SetDescriptorHeaps(2, descriptorHeaps);

				if (NewState.m_PipelineState->GetMetaData().m_Type == PIPELINE_STATE_TYPE_COMPUTE)
				{
					m_CurrentD3DCommandList->SetComputeRootSignature(NewState.m_RootSignature->GetD3DRootSignature());
				}
				else if (NewState.m_PipelineState->GetMetaData().m_Type == PIPELINE_STATE_TYPE_DEFAULT)
				{
					m_CurrentD3DCommandList->SetGraphicsRootSignature(NewState.m_RootSignature->GetD3DRootSignature());
				}
			}

			if (CurrentState.m_VertexBuffers != NewState.m_VertexBuffers || CurrentState.m_VertexBufferOffsets != NewState.m_VertexBufferOffsets || CurrentState.m_VertexBufferStrides != NewState.m_VertexBufferStrides)
			{
				std::vector<D3D12_VERTEX_BUFFER_VIEW> bufferViews;
				for (uint32_t i = 0; i < NewState.m_VertexBuffers.size(); ++i)
				{
					const Ref<Buffer>& buffer = NewState.m_VertexBuffers[i];
					const uint64_t offset = NewState.m_VertexBufferOffsets.empty() ? 0 : NewState.m_VertexBufferOffsets[i];
					const uint64_t size = NewState.m_VertexBufferSizes.empty() ? (buffer->GetDesc().m_ElementSize * buffer->GetDesc().m_ElementCount) : NewState.m_VertexBufferSizes[i];
					const uint32_t stride = NewState.m_VertexBufferStrides.empty() ? buffer->GetDesc().m_ElementSize : NewState.m_VertexBufferStrides[i];

					D3D12_VERTEX_BUFFER_VIEW view;
					view.BufferLocation = buffer->GetD3DResource()->GetGPUVirtualAddress() + offset;
					view.SizeInBytes = size;
					view.StrideInBytes = stride;
					bufferViews.push_back(view);
				}
				m_CurrentD3DCommandList->IASetVertexBuffers(0, bufferViews.size(), bufferViews.data());
			}
			if (CurrentState.m_IndexBuffer != NewState.m_IndexBuffer || CurrentState.m_IndexBufferOffset != NewState.m_IndexBufferOffset || CurrentState.m_IndexBufferFormat != NewState.m_IndexBufferFormat)
			{
				const Ref<Buffer>& buffer = NewState.m_IndexBuffer;
				const uint64_t offset = NewState.m_IndexBufferOffset;
				const uint64_t size = NewState.m_IndexBufferSize == 0 ? (buffer->GetDesc().m_ElementSize * buffer->GetDesc().m_ElementCount) : NewState.m_IndexBufferSize;
				const Format format = NewState.m_IndexBufferFormat != FORMAT_UNKNOWN ? NewState.m_IndexBufferFormat : buffer->GetDesc().m_Format;

				D3D12_INDEX_BUFFER_VIEW view;
				view.BufferLocation = buffer->GetD3DResource()->GetGPUVirtualAddress() + offset;
				view.SizeInBytes = size;
				view.Format = D3DConvertFormat(format);
				m_CurrentD3DCommandList->IASetIndexBuffer(&view);
			}
			if (CurrentState.m_Topology != NewState.m_Topology)
			{
				m_CurrentD3DCommandList->IASetPrimitiveTopology(D3DConvertPrimitiveTopology(NewState.m_Topology));
			}

			for (int i = 0; i < NewState.m_RootCB.size(); i++)
			{
				auto buffer = NewState.m_RootCB[i];
				auto offset = NewState.m_RootCBOffsets[i];
				if ((CurrentState.m_RootCB[i] != buffer) || (CurrentState.m_RootCBOffsets[i] != offset))
				{
					D3D12_GPU_VIRTUAL_ADDRESS addr = buffer->GetD3DResource()->GetGPUVirtualAddress() + NewState.m_RootCBOffsets[i];

					//Aditional buffer bound or different buffer bound at a previously bound slot
					if (NewState.m_PipelineState->GetMetaData().m_Type == PIPELINE_STATE_TYPE_COMPUTE)
						m_CurrentD3DCommandList->SetComputeRootConstantBufferView(i, addr);
					else
						m_CurrentD3DCommandList->SetGraphicsRootConstantBufferView(i, addr);
				}
			}

			if (m_RenderTargetUpdate)
			{
				//Assemble final list of render targets removing null ones
				std::vector<D3D12_CPU_DESCRIPTOR_HANDLE*> FinalRT;
				for (auto Descriptor : NewState.m_RenderTargets)
				{
					FinalRT.push_back(&Descriptor->GetHandle());
				}
				
				m_CurrentD3DCommandList->OMSetRenderTargets(FinalRT.size(),FinalRT.size() ? *FinalRT.data() : nullptr, false, &NewState.m_DepthStencil->GetHandle());
				m_RenderTargetUpdate = false;
			}
			
			CurrentState = NewState;
			m_StateUpdate = false;
		}
	}

	void Context::ClearRenderTargets(Ref<RenderTargetView>* rtvs, size_t numRtvs)
	{
		const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		for (uint32_t i = 0; i < numRtvs; ++i)
		{
			m_CurrentD3DCommandList->ClearRenderTargetView(rtvs[i]->GetHandle(), clearColor, 0, nullptr);
			++m_NumRecordedCommands;
		}
	}

	void Context::ClearDepthStencil(Ref<DepthStencilView> dsv, float clearValue)
	{
		m_CurrentD3DCommandList->ClearDepthStencilView(dsv->GetHandle(), D3D12_CLEAR_FLAG_DEPTH, clearValue, 0, 0, nullptr);
		++m_NumRecordedCommands;
	}

	Ref<Buffer> Context::BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructureBuildDesc& desc)
	{
		Device* device = GetDevice();

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
		std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;
		if (desc.m_Type == RaytracingAccelerationStructureBuildDesc::Type::TopLevel)
		{
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs = buildDesc.Inputs;
			inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
			inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
			inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;

			std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
			for (uint32_t i = 0; i < desc.m_InstanceDescs.size(); ++i)
			{
				const RaytracingInstanceDesc& rtInstanceDesc = desc.m_InstanceDescs[i];
				D3D12_RAYTRACING_INSTANCE_DESC desc = {};
				desc.AccelerationStructure = rtInstanceDesc.m_BLAS->GetD3DResource()->GetGPUVirtualAddress();
				desc.InstanceID = rtInstanceDesc.m_InstanceId;
				desc.InstanceMask = 0xff;
				desc.InstanceContributionToHitGroupIndex = 0;
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
		else if (desc.m_Type == RaytracingAccelerationStructureBuildDesc::Type::BottomLevel)
		{
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs = buildDesc.Inputs;
			inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
			inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
			inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

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

	Context* Context::GetCurrentContext()
	{
		return g_CurrentContext;
	}

	void Context::BindRenderTargets(Ref<RenderTargetView>* rtviews, size_t viewCount)
	{
		std::vector<Ref<RenderTargetView>> rtdescriptors(rtviews, rtviews + viewCount);
		if (NewState.m_RenderTargets != rtdescriptors)
		{
			NewState.m_RenderTargets = rtdescriptors;
			m_StateUpdate = true;
			m_RenderTargetUpdate = true;
		}
	}

	void Context::BindDepthStencil(Ref<DepthStencilView> dsview)
	{
		if (NewState.m_DepthStencil != dsview)
		{
			NewState.m_DepthStencil = dsview;
			m_StateUpdate = true;
			m_RenderTargetUpdate = true;
		}
	}

	void Context::BindVertexBuffers(Ref<Buffer>* buffers, size_t numVertexBuffers, const uint64_t* offsets, const uint32_t* sizes, const uint32_t* strides)
	{
		NewState.m_VertexBuffers = std::vector<Ref<Buffer>>(buffers, buffers + numVertexBuffers);
		NewState.m_VertexBufferOffsets.clear();
		NewState.m_VertexBufferSizes.clear();
		NewState.m_VertexBufferStrides.clear();
		if (offsets)
		{
			NewState.m_VertexBufferOffsets = std::vector<uint64_t>(offsets, offsets + numVertexBuffers);
		}
		if (sizes)
		{
			NewState.m_VertexBufferSizes = std::vector<uint32_t>(sizes, sizes + numVertexBuffers);
		}
		if (strides)
		{
			NewState.m_VertexBufferStrides = std::vector<uint32_t>(strides, strides + numVertexBuffers);
		}
		m_StateUpdate = true;
	}

	void Context::BindIndexBuffer(Ref<Buffer> indexBuffer, const uint64_t offset, const uint32_t size, const Format format)
	{
		NewState.m_IndexBuffer = indexBuffer;
		NewState.m_IndexBufferOffset = offset;
		NewState.m_IndexBufferSize = size;
		NewState.m_IndexBufferFormat = format;
		m_StateUpdate = true;
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
		if (NewState.m_Topology != topologyType)
		{
			NewState.m_Topology = topologyType;
			m_StateUpdate = true;
		}
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