#include "editorrenderer.h"

#if ENABLE_EDITOR

#include "render/device.h"
#include "imgui.h"

namespace vkr::Editor
{

	Renderer::Renderer()
		: m_CurrentRenderDataIndex(0)
	{

	}

	Renderer::~Renderer()
	{

	}

	bool Renderer::Init()
	{
		Render::Device* device = Render::GetDevice();

		const std::string vertexShaderCode =
			"cbuffer PerDrawConstants : register(b0)\n"
			"{\n"
			"	float2 VertexOffset;\n"
			"	float2 VertexScale;\n"
			"	uint TextureDescriptorIndex;\n"
			"	uint3 _pad;\n"
			"};\n"
			"struct VertexInput\n"
			"{\n"
			"	float2 position: POSITION;\n"
			"	float2 uv: UV0;\n"
			"	float4 color: COLOR0;\n"
			"};\n"
			"struct PixelInput\n"
			"{\n"
			"	float4 position: SV_POSITION;\n"
			"	float2 uv: UV0;\n"
			"	float4 color: COLOR0;\n"
			"};\n"
			"PixelInput Main(VertexInput input)\n"
			"{\n"
			"	PixelInput output;\n"
			"	output.position = float4((input.position * VertexScale) + VertexOffset, 0.0, 1.0);\n"
			"	output.uv = input.uv;\n"
			"	output.color = input.color;\n"
			"	return output;\n"
			"};\n";
		Ref<Render::Shader> vertexShader = device->CreateShaderFromString(vertexShaderCode, L"Main", Render::SHADER_STAGE_VERTEX);

		const std::string sdrPixelShaderCode =
			"cbuffer PerDrawConstants : register(b0)\n"
			"{\n"
			"	float2 VertexOffset;\n"
			"	float2 VertexScale;\n"
			"	uint TextureDescriptorIndex;\n"
			"	uint3 _pad;\n"
			"};\n"
			"struct PixelInput\n"
			"{\n"
			"	float4 position: SV_POSITION;\n"
			"	float2 uv: UV0;\n"
			"	float4 color: COLOR0;\n"
			"};\n"
			"SamplerState g_SamplerBilinearClamp : register(s1);\n"
			"float4 Main(PixelInput input) : SV_TARGET0\n"
			"{\n"
			"	Texture2D tex = ResourceDescriptorHeap[TextureDescriptorIndex];\n"
			"	return input.color * tex.Sample(g_SamplerBilinearClamp, input.uv);\n"
			"};\n";
		Ref<Render::Shader> sdrPixelShader = device->CreateShaderFromString(sdrPixelShaderCode, L"Main", Render::SHADER_STAGE_PIXEL);

		//Ref<Render::Shader> hdrPixelShader = Render::GetDevice()->CreateShader();

		Render::PipelineStateDesc sdrPsoDesc = {};
		sdrPsoDesc.m_Type = Render::PIPELINE_STATE_TYPE_DEFAULT;
		sdrPsoDesc.Default.m_VertexShader = vertexShader.get();
		sdrPsoDesc.Default.m_PixelShader = sdrPixelShader.get();
		sdrPsoDesc.Default.m_VertexLayout.InsertAttribute(Render::VertexAttribute::TYPE_POSITION, Render::FORMAT_RG32_FLOAT);
		sdrPsoDesc.Default.m_VertexLayout.InsertAttribute(Render::VertexAttribute::TYPE_UV, Render::FORMAT_RG32_FLOAT);
		sdrPsoDesc.Default.m_VertexLayout.InsertAttribute(Render::VertexAttribute::TYPE_COLOR, Render::FORMAT_RGBA8_UNORM);
		sdrPsoDesc.Default.m_BlendState.RTBlends[0].m_Enabled = true;
		sdrPsoDesc.Default.m_BlendState.RTBlends[0].m_SrcBlend = Render::BLEND_SRC_ALPHA;
		sdrPsoDesc.Default.m_BlendState.RTBlends[0].m_DstBlend = Render::BLEND_INV_SRC_ALPHA;
		sdrPsoDesc.Default.m_BlendState.RTBlends[0].m_Operation = Render::BLEND_OP_ADD;
		sdrPsoDesc.Default.m_BlendState.RTBlends[0].m_SrcBlendAlpha = Render::BLEND_INV_SRC_ALPHA;
		sdrPsoDesc.Default.m_BlendState.RTBlends[0].m_DstBlendAlpha = Render::BLEND_ZERO;
		sdrPsoDesc.Default.m_BlendState.RTBlends[0].m_AlphaOperation = Render::BLEND_OP_ADD;
		sdrPsoDesc.Default.m_BlendState.RTBlends[0].m_WriteMask = Render::COLOR_WRITE_ALL;
		sdrPsoDesc.Default.m_RenderTargetState.m_Formats[0] = Render::Format::FORMAT_RGB10A2_UNORM;
		sdrPsoDesc.Default.m_RasterizerState.m_CullMode = Render::FACE_CULL_MODE_NONE;
		sdrPsoDesc.Default.m_PrimitiveType = Render::PRIMITIVE_TYPE_TRIANGLE;
		m_SdrShader = device->CreatePipelineState(sdrPsoDesc);

		// Init font texture
		{
			ImGuiIO& io = ImGui::GetIO();

			uint8_t* pixels = nullptr;
			int width = 0;
			int height = 0;
			int bytesPerPixel = 0;
			io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytesPerPixel);

			Render::TextureData::Subresource fontData0 = {};
			fontData0.m_RowPitch = width * bytesPerPixel;
			fontData0.m_SlicePitch = fontData0.m_RowPitch * height;
			fontData0.m_Data.resize(fontData0.m_SlicePitch);
			memcpy(fontData0.m_Data.data(), pixels, fontData0.m_SlicePitch);

			Render::TextureData fontData = {};
			fontData.m_Subresources.push_back(fontData0);

			Render::TextureDesc fontTextureDesc = {};
			fontTextureDesc.m_Dimension = Render::ResourceDimension::Texture2D;
			fontTextureDesc.m_Format = Render::Format::FORMAT_RGBA8_UNORM;
			fontTextureDesc.m_Size = Vector3u(width, height, 1);
			fontTextureDesc.m_MipLevels = 1;
			Ref<Render::Texture> fontTexture = device->CreateTexture(fontTextureDesc, &fontData);
			m_FontTexture = device->CreateTextureView({}, fontTexture);

			io.Fonts->SetTexID(m_FontTexture->GetIndex());
		}

		return true;
	}

	void Renderer::Render()
	{
		m_CurrentRenderDataIndex = (m_CurrentRenderDataIndex + 1) % m_RenderData.size();

		RenderData& prepareData = m_RenderData[m_CurrentRenderDataIndex];
		if (prepareData.m_Event)
		{
			prepareData.m_Event->Wait();
		}
		prepareData.Clear();

		// Copy imgui render data
		ImDrawData* drawData = ImGui::GetDrawData();
		prepareData.m_DrawLists.reserve(drawData->CmdListsCount);
		for (int32_t i = 0; i < drawData->CmdListsCount; ++i)
		{
			prepareData.m_DrawLists.push_back(drawData->CmdLists[i]->CloneOutput());
		}

		prepareData.m_ViewportOffset = { drawData->DisplayPos.x, drawData->DisplayPos.y };
		prepareData.m_ViewportSize = { drawData->DisplaySize.x, drawData->DisplaySize.y };
		prepareData.m_ViewportScale = { drawData->FramebufferScale.x, drawData->FramebufferScale.y };

		const uint32_t renderDataIndex = m_CurrentRenderDataIndex;
		prepareData.m_Event = Render::QueueGraphicsTask([this, renderDataIndex]() { RenderTask(renderDataIndex); });
	}

	void Renderer::SetOutputTarget(Render::RenderTargetView* target)
	{
		m_RenderTarget = target;
	}

	void Renderer::RenderTask(const uint32_t renderDataIndex)
	{
		const RenderData& renderData = m_RenderData[m_CurrentRenderDataIndex];
		if (renderData.m_DrawLists.empty())
		{
			return;
		}

		Render::Context* ctx = Render::Context::GetCurrentContext();

		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = m_RenderTarget->GetTexture();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_ALL;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_RENDER_TARGET;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_RENDER_TARGET;
			ctx->TextureBarrier(barrierDesc);
		}
		ctx->BindRenderTargets(1, &m_RenderTarget);
		ctx->SetPrimitiveTopology(Render::PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		for (uint32_t i = 0; i < renderData.m_DrawLists.size(); ++i)
		{
			ImDrawList* drawList = renderData.m_DrawLists[i];
			
			const uint32_t vtxBufferByteSize = drawList->VtxBuffer.size_in_bytes();
			const uint32_t vtxByteSize = sizeof(ImDrawVert);
			const uint32_t idxBufferByteSize = drawList->IdxBuffer.size_in_bytes();
			Render::TempBuffer vertexBuffer = Render::GetDevice()->GetTempBuffer(Render::TEMP_BUFFER_USAGE_STAGING, vtxBufferByteSize, vtxBufferByteSize, drawList->VtxBuffer.begin());
			Render::TempBuffer indexBuffer = Render::GetDevice()->GetTempBuffer(Render::TEMP_BUFFER_USAGE_STAGING, idxBufferByteSize, idxBufferByteSize, drawList->IdxBuffer.begin());

			ctx->BindVertexBuffer(vertexBuffer.m_Buffer.get(), vertexBuffer.m_Offset, vtxBufferByteSize, vtxByteSize);
			ctx->BindIndexBuffer(indexBuffer.m_Buffer.get(), indexBuffer.m_Offset, idxBufferByteSize, Render::FORMAT_R16_UINT);
			ctx->BindPipelineState(m_SdrShader.get());

			const Vector2f clipScale = renderData.m_ViewportScale;
			const Vector2f clipOffset = renderData.m_ViewportOffset;
			const Vector2f clipSize = renderData.m_ViewportSize * clipScale;
			ctx->SetViewport(0.0f, 0.0f, clipSize.x, clipSize.y);
			for (uint32_t cmdIdx = 0; cmdIdx < drawList->CmdBuffer.size(); ++cmdIdx)
			{
				const ImDrawCmd& cmd = drawList->CmdBuffer[cmdIdx];
				
				const Vector2f clipMin((cmd.ClipRect.x - clipOffset.x), (cmd.ClipRect.y - clipOffset.y));
				const Vector2f clipMax((cmd.ClipRect.z - clipOffset.x), (cmd.ClipRect.w - clipOffset.y));
				if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
					continue;

				ctx->SetScissorRect(clipMin.x, clipMin.y, clipMax.x, clipMax.y); 

				struct PerDrawConstants
				{
					Vector2f VertexOffset;
					Vector2f VertexScale;
					uint32_t TextureDescriptorIndex;
					uint32_t _pad[3];
				};
				PerDrawConstants constants;
				constants.VertexScale.x = 2.0f / clipSize.x;
				constants.VertexScale.y = -2.0f / clipSize.y;
				constants.VertexOffset.x = -1.0f - clipOffset.x * constants.VertexScale.x;
				constants.VertexOffset.y = 1.0f - clipOffset.y * constants.VertexScale.y;
				constants.TextureDescriptorIndex = cmd.TextureId;
				ctx->BindLocalConstantBuffer(sizeof(constants), &constants, 0);

				ctx->DrawIndexed(cmd.ElemCount, cmd.IdxOffset, cmd.VtxOffset);
			}
		}
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = m_RenderTarget->GetTexture();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_ALL;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_PRESENT;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_COMMON;
			ctx->TextureBarrier(barrierDesc);
		}
	}

	void Renderer::RenderData::Clear()
	{
		for (uint32_t i = 0; i < m_DrawLists.size(); ++i)
		{
			IM_DELETE(m_DrawLists[i]);
		}
		m_DrawLists.clear();
	}
}
#endif //ENABLE_EDITOR