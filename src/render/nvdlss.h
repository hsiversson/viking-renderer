#pragma once

namespace vkr::Graphics 
{
	class View;
}

namespace vkr::Render
{
	class Context;
	//Encapsulates the state tracking and implementation of DLSS for one specific view
	class NvDLSS
	{
	public:
		NvDLSS();
		~NvDLSS();
		void Prepare(Graphics::View& view);
		void Upscale(Graphics::View& view, Render::Context* ctx);

	private:
		struct PImpl;
		UniquePtr<PImpl> m_pImpl;
	};

}