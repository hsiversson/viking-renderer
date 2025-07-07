#include "modelloader_gltf.h"
#include "core/types.h"
#include "render/renderstates.h"
#include "graphics/model.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

namespace vkr::Graphics
{
	static bool ProcessModelPart(const cgltf_primitive& primitive, const std::filesystem::path& modelDirectory, ModelDesc::PartDesc& outDesc)
	{
		// Gather material info
		MaterialDesc& materialDesc = outDesc.m_MaterialDesc;
		const cgltf_material* material = primitive.material;
		if (material->has_pbr_metallic_roughness)
		{
			const std::filesystem::path baseColorTexPath = material->pbr_metallic_roughness.base_color_texture.texture->image->uri;
			const std::filesystem::path normalTexPath = material->normal_texture.texture->image->uri;
			const std::filesystem::path metallicRoughnessTexPath = material->pbr_metallic_roughness.metallic_roughness_texture.texture->image->uri;
			const std::filesystem::path emissiveTexPath = material->emissive_texture.texture->image->uri;

			materialDesc.m_TexturePaths.push_back(modelDirectory / baseColorTexPath);
			materialDesc.m_TexturePaths.push_back(modelDirectory / normalTexPath);
			materialDesc.m_TexturePaths.push_back(modelDirectory / metallicRoughnessTexPath);
			materialDesc.m_TexturePaths.push_back(modelDirectory / emissiveTexPath);

			materialDesc.m_FrontCounterClockwise = true;
			materialDesc.m_TwoSided = false;
		}

		// Gather mesh info
		MeshDesc& meshDesc = outDesc.m_MeshDesc;
		switch (primitive.type)
		{
		case cgltf_primitive_type_points:
			meshDesc.m_Topology = Render::PRIMITIVE_TOPOLOGY_POINTLIST;
			break;
		case cgltf_primitive_type_lines:
			meshDesc.m_Topology = Render::PRIMITIVE_TOPOLOGY_LINELIST;
			break;
		case cgltf_primitive_type_line_strip:
			meshDesc.m_Topology = Render::PRIMITIVE_TOPOLOGY_LINESTRIP;
			break;
		case cgltf_primitive_type_triangles:
			meshDesc.m_Topology = Render::PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		case cgltf_primitive_type_triangle_strip:
			meshDesc.m_Topology = Render::PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			break;
		case cgltf_primitive_type_triangle_fan:
			meshDesc.m_Topology = Render::PRIMITIVE_TOPOLOGY_TRIANGLEFAN;
			break;
		default:
			assert(false);
			return false;
		}

		Render::VertexLayout& vertexLayout = meshDesc.m_VertexLayout;
		meshDesc.m_NumVertices = 0;
		for (size_t a = 0; a < primitive.attributes_count; ++a)
		{
			const cgltf_attribute& attr = primitive.attributes[a];
			const cgltf_accessor* accessor = attr.data;
			meshDesc.m_NumVertices = std::max(static_cast<uint32_t>(accessor->count), meshDesc.m_NumVertices);
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
					const float* v = (float*)(base + i * accessor->stride);
					const Vector3f pos = Vector3f(v[0], v[1], -v[2]);
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
					const float* v = (float*)(base + i * accessor->stride);
					const Vector3f norm = Vector3f(v[0], v[1], -v[2]);
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

				std::vector<uint8_t>& tangentData = meshDesc.m_VertexData[Render::VertexAttribute::TYPE_TANGENT];
				tangentData.reserve(tangentData.size() + (accessor->count * sizeof(Vector3f)));
				cgltf_buffer_view* view = accessor->buffer_view;
				const uint8_t* base = static_cast<const uint8_t*>(view->buffer->data) + view->offset + accessor->offset;
				for (size_t i = 0; i < accessor->count; ++i)
				{
					const float* v = (float*)(base + i * accessor->stride);
					const Vector3f tangent = Vector3f(v[0], v[1], -v[2]);
					tangentData.insert(tangentData.end(), (uint8_t*)&tangent, (uint8_t*)&tangent + sizeof(Vector3f));
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

				std::vector<uint8_t>& uvData = meshDesc.m_VertexData[Render::VertexAttribute::TYPE_UV];
				uvData.reserve(uvData.size() + (accessor->count * sizeof(Vector2f)));
				cgltf_buffer_view* view = accessor->buffer_view;
				const uint8_t* base = static_cast<const uint8_t*>(view->buffer->data) + view->offset + accessor->offset;
				for (size_t i = 0; i < accessor->count; ++i)
				{
					const float* v = (float*)(base + i * accessor->stride);
					const Vector2f uv = Vector2f(v[0], v[1]);
					uvData.insert(uvData.end(), (uint8_t*)&uv, (uint8_t*)&uv + sizeof(Vector2f));
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

				std::vector<uint8_t>& boneIndexData = meshDesc.m_VertexData[Render::VertexAttribute::TYPE_BONE_INDEX];
				boneIndexData.reserve(boneIndexData.size() + (accessor->count * sizeof(Vector4u16)));
				cgltf_buffer_view* view = accessor->buffer_view;
				const uint8_t* base = static_cast<const uint8_t*>(view->buffer->data) + view->offset + accessor->offset;
				for (size_t i = 0; i < accessor->count; ++i)
				{
					const uint16_t* v = (uint16_t*)(base + i * accessor->stride);
					const Vector4u16 boneIndex = Vector4u16(v[0], v[1], v[2], v[3]);
					boneIndexData.insert(boneIndexData.end(), (uint8_t*)&boneIndex, (uint8_t*)&boneIndex + sizeof(Vector4u16));
				}
			}
			else if (attr.type == cgltf_attribute_type_weights)
			{
				Render::VertexAttribute attribute;
				attribute.m_Type = Render::VertexAttribute::TYPE_BONE_WEIGHT;
				attribute.m_Index = attr.index;
				attribute.m_BufferSlot = 0;
				attribute.m_Format = Render::FORMAT_RGBA16_FLOAT;
				vertexLayout.m_Attributes.insert(attribute);

				std::vector<uint8_t>& boneWeightData = meshDesc.m_VertexData[Render::VertexAttribute::TYPE_BONE_INDEX];
				boneWeightData.reserve(boneWeightData.size() + (accessor->count * sizeof(Vector4f)));
				cgltf_buffer_view* view = accessor->buffer_view;
				const uint8_t* base = static_cast<const uint8_t*>(view->buffer->data) + view->offset + accessor->offset;
				for (size_t i = 0; i < accessor->count; ++i)
				{
					const float* v = (float*)(base + i * accessor->stride);
					const Vector4f boneWeight = Vector4f(v[0], v[1], v[2], v[3]);
					boneWeightData.insert(boneWeightData.end(), (uint8_t*)&boneWeight, (uint8_t*)&boneWeight + sizeof(Vector4f));
				}
			}
		}

		if (primitive.indices)
		{
			const cgltf_accessor* accessor = primitive.indices;
			cgltf_buffer_view* view = accessor->buffer_view;
			const uint8_t* base = static_cast<const uint8_t*>(view->buffer->data) + view->offset + accessor->offset;

			const uint32_t bytesPerIdx = accessor->component_type == cgltf_component_type_r_32u ? 4 : 2;
			const uint32_t stride = view->stride ? view->stride : bytesPerIdx;

			meshDesc.m_NumIndices = static_cast<uint32_t>(accessor->count);
			switch (accessor->component_type)
			{
			case cgltf_component_type_r_32u:
				meshDesc.m_IndexFormat = Render::FORMAT_R32_UINT;
				break;
			case cgltf_component_type_r_16u:
				meshDesc.m_IndexFormat = Render::FORMAT_R16_UINT;
				break;
			default:
				//std::cerr << "Unsupported index format\n";
				assert(false);
				return false;
			}

			meshDesc.m_IndexData.resize(accessor->count * bytesPerIdx);

			const uint8_t* src = static_cast<const uint8_t*>(view->buffer->data) + view->offset + accessor->offset;
			uint8_t* dst = meshDesc.m_IndexData.data();
			if (stride == bytesPerIdx)
			{
				// tightly packed → one bulk copy
				memcpy(dst, src, static_cast<size_t>(accessor->count) * bytesPerIdx);
			}
			else
			{
				// interleaved / padded → copy per index
				for (uint32_t i = 0; i < accessor->count; ++i)
				{
					memcpy(dst + i * bytesPerIdx, src + i * stride, bytesPerIdx);
				}
			}
		}

		return true;
	}

	static bool ProcessNode(const cgltf_node* node, const std::filesystem::path& modelDirectory, const Mat44& parentLocalTransform, std::vector<ModelDesc::PartDesc>& outPartDescs)
	{
		Mat44 evaluatedLocalTransform;
		{
			float mat[16];
			cgltf_node_transform_local(node, mat); 
			Mat44 localTransform;
			memcpy(localTransform.m, mat, sizeof(mat));
			evaluatedLocalTransform = parentLocalTransform * localTransform;
		}

		ModelDesc::PartDesc partDesc = {};
		const bool hasMesh = node->mesh;
		if (hasMesh)
		{
			assert(node->mesh->primitives_count <= 1);
			for (uint32_t i = 0; i < node->mesh->primitives_count; ++i)
			{
				const cgltf_primitive& primitive = node->mesh->primitives[i];
				ProcessModelPart(primitive, modelDirectory, partDesc);
			}

			partDesc.m_LocalTransform = evaluatedLocalTransform;
		}

		for (uint32_t i = 0; i < node->children_count; ++i)
		{
			ProcessNode(node->children[i], modelDirectory, evaluatedLocalTransform, hasMesh ? partDesc.m_ChildDescs : outPartDescs);
		}

		if (hasMesh)
			outPartDescs.push_back(partDesc);

		return true;
	}

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
			return nullptr;

		if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success)
		{
			cgltf_free(data);
			return nullptr;
		}

		ModelDesc modelDesc;
		
		// traverse nodes
		cgltf_scene* scene = data->scene;
		for (uint32_t i = 0; i < scene->nodes_count; ++i)
		{
			ProcessNode(scene->nodes[i], filepath.parent_path(), Mat44::Identity(), modelDesc.m_PartDescs);
		}

		cgltf_free(data);

		Ref<Model> model = MakeRef<Model>();
		if (!model->Init(modelDesc))
			return nullptr;

		return model;
	}

}