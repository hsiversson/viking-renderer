#pragma once
#include "render/deviceobject.h"
#include "render/rendercommon.h"
#include "render/event.h"

namespace vkr::Render
{
	class Texture;
	class CommandQueue;
	class SwapChain : public DeviceObject
	{
		static constexpr uint32_t NumBackBuffers = 2;

	public:
		SwapChain(Device& device);
		~SwapChain();

		bool Init(void* nativeWindowHandle, const Vector2u& size);

		void Resize(const Vector2u& newSize);
		void SetHdrEnabled(bool hdr);
		bool IsHdrEnabled() const;

		Ref<Texture> GetOutputTexture();

		void Present();

	private:
		void CreateResources();
		void ReleaseResources();

	private:
		ComPtr<IDXGISwapChain1> m_SwapChain;
		ComPtr<IDXGISwapChain4> m_SwapChain4;
		ComPtr<IDXGIOutput6> m_Output;
		Ref<CommandQueue> m_CommandQueue; //Graphics queue ref. Needed to insert fences during present

		struct BackBufferResource
		{
			Event m_LastFrameEvent;
			Ref<Texture> m_Texture;
		};
		std::array<BackBufferResource, NumBackBuffers> m_BackBuffers;
		uint32_t m_CurrentBackBufferIndex;

		bool m_IsHdrEnabled;
		bool m_HdrSupported;
	};
}