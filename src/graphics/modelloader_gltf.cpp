#include "modelloader_gltf.h"
#include "utils/types.h"
#include "render/renderstates.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

namespace vkr::Graphics
{

	ModelLoader_GLTF::ModelLoader_GLTF()
	{

	}

	ModelLoader_GLTF::~ModelLoader_GLTF()
	{

	}

	bool ModelLoader_GLTF::Load(const std::filesystem::path& filepath)
	{
		const std::string path = filepath.string();
		cgltf_options options = {};
		cgltf_data* data = nullptr;
		if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success)
			return false;

		if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success)
		{
			cgltf_free(data);
			return false;
		}

		std::vector<Vector3f> vertexPositions;
		std::vector<Vector3f> vertexNormals;
		std::vector<Vector3f> vertexTangents;
		std::vector<Vector3f> vertexBoneWeights;
		std::vector<Vector4f> vertexColors;
		std::vector<Vector2f> vertexUvs;

		std::vector<uint32_t> indices;

		if (data->meshes_count > 0)
		{
			for (uint32_t i = 0; i < data->meshes_count; ++i)
			{
				const cgltf_mesh& mesh = data->meshes[i];
				for (uint32_t primitiveIdx = 0; primitiveIdx < mesh.primitives_count; ++primitiveIdx)
				{
					const cgltf_primitive& primitive = mesh.primitives[primitiveIdx];
					
					std::unordered_set<Render::VertexAttribute> usedAttributes;
					for (size_t a = 0; a < primitive.attributes_count; ++a) 
					{
						const cgltf_attribute& attr = primitive.attributes[a];
						const cgltf_accessor* accessor = attr.data;

						if (attr.type == cgltf_attribute_type_position) 
						{
							Render::VertexAttribute attribute;
							attribute.m_Type = Render::VertexAttribute::TYPE_POSITION;
							attribute.m_Index = attr.index;
							attribute.m_BufferSlot = 0;
							attribute.m_Format = DXGI_FORMAT_R32G32B32_FLOAT;
							usedAttributes.insert(attribute);

							vertexPositions.resize(accessor->count);
							cgltf_buffer_view* view = accessor->buffer_view;
							const uint8_t* base = static_cast<const uint8_t*>(view->buffer->data) + view->offset + accessor->offset;
							for (size_t i = 0; i < accessor->count; ++i) 
							{
								float* v = (float*)(base + i * accessor->stride);
								vertexPositions[i] = Vector3f(v[0], v[1], v[2]);
							}
						}
						else if (attr.type == cgltf_attribute_type_normal) 
						{
							vertexNormals.resize(accessor->count);
							cgltf_buffer_view* view = accessor->buffer_view;
							const uint8_t* base = static_cast<const uint8_t*>(view->buffer->data) + view->offset + accessor->offset;
							for (size_t i = 0; i < accessor->count; ++i) 
							{
								float* v = (float*)(base + i * accessor->stride);
								vertexNormals[i] = Vector3f(v[0], v[1], v[2]);
							}
						}
						else if (attr.type == cgltf_attribute_type_texcoord) 
						{
							vertexUvs.resize(accessor->count);
							cgltf_buffer_view* view = accessor->buffer_view;
							const uint8_t* base = static_cast<const uint8_t*>(view->buffer->data) + view->offset + accessor->offset;
							for (size_t i = 0; i < accessor->count; ++i) 
							{
								float* v = (float*)(base + i * accessor->stride);
								vertexUvs[i] = Vector2(v[0], v[1]);
							}
						}
					}
				}
			}
		}

		cgltf_free(data);

		return true;
	}

}