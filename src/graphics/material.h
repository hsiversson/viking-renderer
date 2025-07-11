#pragma once
#include "core/types.h"
#include "render/renderstates.h"

#include <variant>

namespace vkr::Render
{
	class TextureView;
	class Sampler;
	class PipelineState;
	class Shader;
}

namespace vkr::Graphics
{
	enum class MaterialType
	{
		Surface,
		Volume
	};

	enum class MaterialBlendMode
	{
		Opaque,
		AlphaTested,
		Translucent
	};

	enum class MaterialParameterType
	{
		StaticBool,
		Float,
		Float2,
		Float3,
		Float4,
		Texture,
		Sampler,

		Count,
		Undefined = Count,
	};

	struct MaterialParameterDesc
	{
		std::string m_Identifier;
		MaterialParameterType m_Type = MaterialParameterType::Undefined;

		MaterialParameterDesc() = default;
		MaterialParameterDesc(const std::string& identifier, MaterialParameterType type)
			: m_Identifier(identifier)
			, m_Type(type)
		{
		}
	};

	struct MaterialParameterValue
	{
		using ValueType = std::variant<bool, float, Vector2f, Vector3f, Vector4f, Ref<Render::TextureView>, Ref<Render::Sampler>>;
		static_assert(static_cast<uint32_t>(MaterialParameterType::Count) == 7);

		ValueType m_Value;

		MaterialParameterValue() = default;

		template<typename T>
		MaterialParameterValue(const T& value) 
			: m_Value(value) 
		{}

		template<typename T>
		void Set(const T& value) 
		{ 
			m_Value = value; 
		}

		template<typename T>
		const T& Get() const 
		{ 
			return std::get<T>(m_Value); 
		}

		MaterialParameterType GetType() const
		{
			return static_cast<MaterialParameterType>(m_Value.index());
		}
	};

	struct MaterialDesc
	{
		std::vector<std::pair<MaterialParameterDesc, MaterialParameterValue>> m_Parameters;

		MaterialBlendMode m_BlendMode = MaterialBlendMode::Opaque;
		MaterialType m_Type = MaterialType::Surface;

		bool m_WriteVelocity;
		bool m_TwoSided;
		bool m_FrontCounterClockwise;
	};

	class Material
	{
	public:
		Material();
		~Material();

		bool Init(const MaterialDesc& desc);

		Ref<Render::PipelineState> GetDepthPipelineState(const Render::VertexLayout& vertexLayout);
		Ref<Render::PipelineState> GetDefaultPipelineState(const Render::VertexLayout& vertexLayout);

		void AddParameter(const MaterialParameterDesc& desc, const MaterialParameterValue& defaultValue);
		const MaterialParameterDesc* FindParameter(const std::string& identifier) const;
		const MaterialParameterValue* GetParameterValue(const std::string& identifier) const;
		const std::vector<MaterialParameterDesc>& GetParameters() const;

	private:
		Ref<Render::PipelineState> GetOrCreatePSO(const Render::VertexLayout& vertexLayout, bool depthOnly);

	private:
		Ref<Render::Shader> m_PixelShader;

		using CachedPSOs = std::unordered_map<Render::VertexLayout, Ref<Render::PipelineState>>;
		CachedPSOs m_DefaultPSOs;
		CachedPSOs m_DepthOnlyPSOs;

		std::vector<MaterialParameterDesc> m_Parameters;
		std::unordered_map<std::string, MaterialParameterValue> m_ParameterValues;

		MaterialDesc m_Desc;
	};

	class MaterialInstance
	{
	public:
		MaterialInstance(const Ref<Material>& material);

		void SetParameterValue(const std::string& identifier, const MaterialParameterValue& value);
		const MaterialParameterValue* GetParameterValue(const std::string& identifier) const;
		const MaterialParameterDesc* GetParameter(const std::string& identifier) const;
		const std::vector<MaterialParameterDesc>& GetParameters() const;

	private:
		Ref<Material> m_Material;
		std::unordered_map<std::string, MaterialParameterValue> m_ParameterOverrides;
	};
};