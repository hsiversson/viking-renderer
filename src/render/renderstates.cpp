#include "renderstates.h"

namespace vkr::Render
{
	const char* VertexAttribute::GetTypeSemantic(Type type)
	{
		switch (type)
		{
		case TYPE_POSITION:
			return "POSITION";
		case TYPE_NORMAL:
			return "NORMAL";
		case TYPE_TANGENT:
			return "TANGENT";
		case TYPE_UV:
			return "UV";
		case TYPE_COLOR:
			return "COLOR";
		case TYPE_BONE_INDEX:
			return "BONE_INDEX";
		case TYPE_BONE_WEIGHT:
			return "BONE_WEIGHT";
		default:
			assert(false);
			return nullptr;
		}
	}

	uint32_t VertexLayout::GetStride() const
	{
		return m_Stride;
	}

	void VertexLayout::InsertAttribute(VertexAttribute::Type type, Format format, uint32_t index, uint32_t bufferSlot)
	{
		uint32_t size = GetFormatBytesPerPixel(format);
		VertexAttribute attr = { type, index, bufferSlot, format, size};
		
		if (m_Attributes.insert(attr).second)
		{
			m_Stride += size;
		}
	}

	uint32_t VertexLayout::GetByteOffset(VertexAttribute::Type type, uint32_t index) const
	{
		uint32_t currentBufferSlot = 0;
		uint32_t accumulated = 0;
		for (auto& attr : m_Attributes)
		{
			if (currentBufferSlot != attr.m_BufferSlot)
			{
				accumulated = 0;
				currentBufferSlot = attr.m_BufferSlot;
			}
			if ((attr.m_Type == type) && (attr.m_Index == index))
				return accumulated;
			accumulated += attr.m_Size;
		}
		return accumulated;
	}

	void GetDefaultRasterizerState(RasterizerState& outRasterizerState)
	{
		outRasterizerState.m_CullMode = FACE_CULL_MODE_BACK;
		outRasterizerState.m_Wireframe = false;
		outRasterizerState.m_AntialiasedLine = false;
		outRasterizerState.m_FrontIsCounterClockwise = false;
	}

	void GetGreaterEqualDepthStencilState(DepthStencilState& outDepthStencilState, bool writeDepth)
	{
		outDepthStencilState.m_Enabled = true;
		outDepthStencilState.m_WriteDepth = writeDepth;
		outDepthStencilState.m_ComparisonFunc = COMPARISON_FUNC_GREATER_EQUAL;
		outDepthStencilState.m_DSFormat = FORMAT_D32_FLOAT;
	}

	void GetGreaterDepthStencilState(DepthStencilState& outDepthStencilState, bool writeDepth)
	{
		outDepthStencilState.m_Enabled = true;
		outDepthStencilState.m_WriteDepth = writeDepth;
		outDepthStencilState.m_ComparisonFunc = COMPARISON_FUNC_GREATER;
		outDepthStencilState.m_DSFormat = FORMAT_D32_FLOAT;
	}

	void GetEqualDepthStencilState(DepthStencilState& outDepthStencilState, bool writeDepth)
	{
		outDepthStencilState.m_Enabled = true;
		outDepthStencilState.m_WriteDepth = writeDepth;
		outDepthStencilState.m_ComparisonFunc = COMPARISON_FUNC_EQUAL;
		outDepthStencilState.m_DSFormat = FORMAT_D32_FLOAT;
	}

}