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