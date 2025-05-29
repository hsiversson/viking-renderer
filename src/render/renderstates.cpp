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
}