#include "modelloader_gltf.h"
#include "utils/types.h"
#include "render/renderstates.h"
#include "graphics/model.h"

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

	Ref<Model> ModelLoader_GLTF::Load(const std::filesystem::path& filepath)
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

		ModelDesc modelDesc = {};

		std::vector<Vector3f> vertexPositions;
		std::vector<Vector3f> vertexNormals;
		std::vector<Vector3f> vertexTangents;
		std::vector<Vector4u16> vertexBoneIndices;
		std::vector<Vector4f> vertexBoneWeights;
		std::vector<Vector4f> vertexColors;
		std::vector<Vector2f> vertexUvs;

		std::vector<uint32_t> indices;

		if (data->meshes_count > 0)
		{
			modelDesc.m_MeshDescs.resize(data->meshes_count);
			for (uint32_t i = 0; i < data->meshes_count; ++i)
			{
				MeshDesc& meshDesc = modelDesc.m_MeshDescs[i];

				const cgltf_mesh& mesh = data->meshes[i];
				for (uint32_t primitiveIdx = 0; primitiveIdx < mesh.primitives_count; ++primitiveIdx)
				{
					const cgltf_primitive& primitive = mesh.primitives[primitiveIdx];

					Render::VertexLayout& vertexLayout = meshDesc.m_VertexLayout;
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
							attribute.m_Format = Render::FORMAT_RGB32_FLOAT;
							vertexLayout.m_Attributes.insert(attribute);

							std::vector<uint8_t>& positionData = meshDesc.m_VertexData[Render::VertexAttribute::TYPE_POSITION];
							positionData.reserve(positionData.size() + (accessor->count * sizeof(Vector3f)));
							cgltf_buffer_view* view = accessor->buffer_view;
							const uint8_t* base = static_cast<const uint8_t*>(view->buffer->data) + view->offset + accessor->offset;
							for (size_t i = 0; i < accessor->count; ++i) 
							{
								const size_t offset = i * sizeof(Vector3f);
								const float* v = (float*)(base + i * accessor->stride);
								const Vector3f pos = Vector3f(v[0], v[1], v[2]);
								positionData.insert(positionData.end(), (uint8_t*)&pos, (uint8_t*)&pos + sizeof(Vector3f));
							}
						}
						else if (attr.type == cgltf_attribute_type_normal) 
						{
							Render::VertexAttribute attribute;
							attribute.m_Type = Render::VertexAttribute::TYPE_NORMAL;
							attribute.m_Index = attr.index;
							attribute.m_BufferSlot = 0;
							attribute.m_Format = Render::FORMAT_RGB32_FLOAT;
							vertexLayout.m_Attributes.insert(attribute);

							std::vector<uint8_t>& normalData = meshDesc.m_VertexData[Render::VertexAttribute::TYPE_NORMAL];
							normalData.reserve(normalData.size() + (accessor->count * sizeof(Vector3f)));
							cgltf_buffer_view* view = accessor->buffer_view;
							const uint8_t* base = static_cast<const uint8_t*>(view->buffer->data) + view->offset + accessor->offset;
							for (size_t i = 0; i < accessor->count; ++i) 
							{
								const size_t offset = i * sizeof(Vector3f);
								const float* v = (float*)(base + i * accessor->stride);
								const Vector3f norm = Vector3f(v[0], v[1], v[2]);
								normalData.insert(normalData.end(), (uint8_t*)&norm, (uint8_t*)&norm + sizeof(Vector3f));
							}
						}
						else if (attr.type == cgltf_attribute_type_tangent)
						{
							Render::VertexAttribute attribute;
							attribute.m_Type = Render::VertexAttribute::TYPE_TANGENT;
							attribute.m_Index = attr.index;
							attribute.m_BufferSlot = 0;
							attribute.m_Format = Render::FORMAT_RGB32_FLOAT;
							vertexLayout.m_Attributes.insert(attribute);

							vertexNormals.resize(accessor->count);
							cgltf_buffer_view* view = accessor->buffer_view;
							const uint8_t* base = static_cast<const uint8_t*>(view->buffer->data) + view->offset + accessor->offset;
							for (size_t i = 0; i < accessor->count; ++i)
							{
								const float* v = (float*)(base + i * accessor->stride);
								vertexNormals[i] = Vector3f(v[0], v[1], v[2]);
							}
						}
						else if (attr.type == cgltf_attribute_type_texcoord) 
						{
							Render::VertexAttribute attribute;
							attribute.m_Type = Render::VertexAttribute::TYPE_UV;
							attribute.m_Index = attr.index;
							attribute.m_BufferSlot = 0;
							attribute.m_Format = Render::FORMAT_RG32_FLOAT;
							vertexLayout.m_Attributes.insert(attribute);

							vertexUvs.resize(accessor->count);
							cgltf_buffer_view* view = accessor->buffer_view;
							const uint8_t* base = static_cast<const uint8_t*>(view->buffer->data) + view->offset + accessor->offset;
							for (size_t i = 0; i < accessor->count; ++i) 
							{
								const float* v = (float*)(base + i * accessor->stride);
								vertexUvs[i] = Vector2(v[0], v[1]);
							}
						}
						else if (attr.type == cgltf_attribute_type_joints)
						{
							Render::VertexAttribute attribute;
							attribute.m_Type = Render::VertexAttribute::TYPE_BONE_INDEX;
							attribute.m_Index = attr.index;
							attribute.m_BufferSlot = 0;
							attribute.m_Format = Render::FORMAT_RGBA16_UINT;
							vertexLayout.m_Attributes.insert(attribute);

							vertexBoneIndices.resize(accessor->count);
							cgltf_buffer_view* view = accessor->buffer_view;
							const uint8_t* base = static_cast<const uint8_t*>(view->buffer->data) + view->offset + accessor->offset;
							for (size_t i = 0; i < accessor->count; ++i)
							{
								const uint16_t* v = (uint16_t*)(base + i * accessor->stride);
								vertexBoneIndices[i] = Vector4u16(v[0], v[1], v[2], v[3]);
							}
						}
						else if (attr.type == cgltf_attribute_type_weights)
						{
							Render::VertexAttribute attribute;
							attribute.m_Type = Render::VertexAttribute::TYPE_BONE_INDEX;
							attribute.m_Index = attr.index;
							attribute.m_BufferSlot = 0;
							attribute.m_Format = Render::FORMAT_RGBA16_FLOAT;
							vertexLayout.m_Attributes.insert(attribute);

							vertexBoneIndices.resize(accessor->count);
							cgltf_buffer_view* view = accessor->buffer_view;
							const uint8_t* base = static_cast<const uint8_t*>(view->buffer->data) + view->offset + accessor->offset;
							for (size_t i = 0; i < accessor->count; ++i)
							{
								const float* v = (float*)(base + i * accessor->stride);
								vertexBoneWeights[i] = Vector4f(v[0], v[1], v[2], v[3]);
							}
						}
					}

					if (primitive.indices)
					{
						const cgltf_accessor* accessor = primitive.indices;
						cgltf_buffer_view* view = accessor->buffer_view;
						const uint8_t* base = static_cast<const uint8_t*>(view->buffer->data) + view->offset + accessor->offset;

						indices.reserve(accessor->count);
						for (size_t i = 0; i < accessor->count; ++i) 
						{
							uint32_t index = 0;
							switch (accessor->component_type) 
							{
							case cgltf_component_type_r_16u:
								index = ((uint16_t*)base)[i];
								break;
							case cgltf_component_type_r_32u:
								index = ((uint32_t*)base)[i];
								break;
							case cgltf_component_type_r_8u:
								index = ((uint8_t*)base)[i];
								break;
							default:
								//std::cerr << "Unsupported index format\n";
								break;
							}
							indices.push_back(index);
						}
					}
				}
			}
		}

		cgltf_free(data);

		return true;
	}

}