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

	}

	void Renderer::RenderData::Clear()
	{
		for (uint32_t i = 0; i < m_DrawLists.size(); ++i)
		{
			IM_DELETE(m_DrawLists[i]);
		}
		m_DrawLists.clear();
		m_Vertices.clear();
		m_Indices.clear();
	}
}
#endif //ENABLE_EDITOR