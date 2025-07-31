#pragma once

namespace vkr::Graphics
{
	class Material;
	class MaterialCompiler
	{
	public:
		MaterialCompiler();
		~MaterialCompiler();

		bool Compile(Material& outMaterial);

	private:
		bool CompileRaytracingHitGroup(Material& outMaterial);

	};
}