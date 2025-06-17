#pragma once

#include "render/rendercommon.h"

#include <functional>

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

		bool operator==(const VertexAttribute& other) const = default;
		bool operator<(const VertexAttribute& other) const { return m_Type < other.m_Type; }
	};

	struct VertexLayout
	{
		std::set<VertexAttribute> m_Attributes;
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
		Format m_DSFormat;
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

namespace std
{
	template <>
	struct hash<vkr::Render::VertexAttribute>
	{
		size_t operator()(const vkr::Render::VertexAttribute& attr) const noexcept
		{
			size_t h = 0;
			HashCombine(h, static_cast<uint8_t>(attr.m_Type));
			HashCombine(h, attr.m_Index);
			HashCombine(h, attr.m_BufferSlot);
			HashCombine(h, static_cast<std::underlying_type_t<vkr::Render::Format>>(attr.m_Format));
			return h;
		}

	private:
		template <typename T>
		static void HashCombine(size_t& seed, const T& val)
		{
			seed ^= std::hash<T>{}(val)+0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
	};
}