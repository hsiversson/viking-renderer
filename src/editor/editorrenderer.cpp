#include "editorrenderer.h"

#if ENABLE_EDITOR

#include "render/device.h"
#include "imgui.h"

namespace vkr::Editor
{

	Renderer::Renderer()
	{

	}

	Renderer::~Renderer()
	{

	}

	bool Renderer::Init()
	{
		//Ref<Render::Shader> vertexShader = Render::GetDevice()->CreateShader();
		//Ref<Render::Shader> sdrPixelShader = Render::GetDevice()->CreateShader();
		//Ref<Render::Shader> hdrPixelShader = Render::GetDevice()->CreateShader();
		//
		//m_SdrShader = Render::GetDevice()->CreatePipelineState();
		//m_HdrShader = Render::GetDevice()->CreatePipelineState();

		return false;
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
		ImGui::Render();
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

	void Renderer::RenderTask(const uint32_t renderDataIndex)
	{
		Render::Context* ctx = Render::Context::GetCurrentContext();
		const RenderData& renderData = m_RenderData[m_CurrentRenderDataIndex];

		for (uint32_t i = 0; i < renderData.m_DrawLists.size(); ++i)
		{
			ImDrawList* drawList = renderData.m_DrawLists[i];
			
			Render::TempBuffer vertexBuffer = Render::GetDevice()->GetTempBuffer(Render::TEMP_BUFFER_USAGE_STAGING, drawList->VtxBuffer.size_in_bytes(), drawList->VtxBuffer.size_in_bytes(), drawList->VtxBuffer.begin());
			Render::TempBuffer indexBuffer = Render::GetDevice()->GetTempBuffer(Render::TEMP_BUFFER_USAGE_STAGING, drawList->IdxBuffer.size_in_bytes(), drawList->IdxBuffer.size_in_bytes(), drawList->IdxBuffer.begin());

			ctx->BindVertexBuffers(&vertexBuffer.m_Buffer, 1, &vertexBuffer.m_Offset);
			ctx->BindIndexBuffer(indexBuffer.m_Buffer, indexBuffer.m_Offset);
			ctx->BindPSO(m_SdrShader);

			const Vector2f clipOffset = renderData.m_ViewportOffset;
			const Vector2f clipScale = renderData.m_ViewportScale;
			for (uint32_t cmdIdx = 0; cmdIdx < drawList->CmdBuffer.size(); ++cmdIdx)
			{
				const ImDrawCmd& cmd = drawList->CmdBuffer[cmdIdx];
				
				const Vector2f clipMin((cmd.ClipRect.x - clipOffset.x) * clipScale.x, (cmd.ClipRect.y - clipOffset.y) * clipScale.y);
				const Vector2f clipMax((cmd.ClipRect.z - clipOffset.x) * clipScale.x, (cmd.ClipRect.w - clipOffset.y) * clipScale.y);
				if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
					continue;

				ctx->SetScissorRect(clipMin.x, clipMin.y, clipMax.x, clipMax.y);

				// TODO: Grab texid and put in constants
				const uint32_t textureDescriptorIndex = cmd.TextureId;

				ctx->DrawIndexed(cmd.ElemCount, cmd.IdxOffset, cmd.VtxOffset);
			}
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