#pragma once

#include "render/rendercommon.h"

namespace vkr::Render
{
	struct VertexAttribute
	{
		enum Type : uint8_t
		{
			TYPE_POSITION,
			TYPE_NORMAL,
			TYPE_TANGENT,
			TYPE_UV,
			TYPE_COLOR,
			TYPE_BONE_INDEX,
			TYPE_BONE_WEIGHT,
		};

		static const char* GetTypeSemantic(Type type);

		Type m_Type;
		uint32_t m_Index;
		uint32_t m_BufferSlot;
		Format m_Format;
	};

	struct VertexLayout
	{
		// We should probably be using a hash set or something to make sure we only have one occurance per attribute.
		std::vector<VertexAttribute> m_Attributes;
	};

	struct RasterizerState
	{
		FaceCullMode m_CullMode;
		bool m_FrontIsCounterClockwise;
		bool m_Wireframe;
		bool m_AntialiasedLine;
	};

	struct DepthStencilState
	{
		bool m_Enabled;
		bool m_WriteDepth;
		ComparisonFunc m_ComparisonFunc;
	};

	struct RenderTargetState
	{
		std::array<Format, MAX_NUM_RENDER_TARGETS> m_Formats;
	};

	struct BlendState
	{
		// should we wrap the things we need?
		D3D12_BLEND_DESC m_D3DBlendDesc;
	};
}