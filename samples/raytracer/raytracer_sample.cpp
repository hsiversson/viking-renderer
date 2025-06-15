#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "utils/types.h"
#include "utils/commandline.h"
#include "utils/meshutils.h"

#include "render/window.h"
#include "render/device.h"
#include "render/pipelinestate.h"
#include "render/resourcedescriptor.h"
#include "render/shader.h"

#include "graphics/camera.h"
#include "graphics/material.h"
#include "graphics/model.h"
#include "graphics/modelobject.h"
#include "graphics/scene.h"
#include "graphics/view.h"
#include "graphics/viewrenderer.h"

using namespace vkr;

D3D12_BLEND_DESC CreateDefaultBlendDesc()
{
	D3D12_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;

	D3D12_RENDER_TARGET_BLEND_DESC rtBlendDesc = {};
	rtBlendDesc.BlendEnable = TRUE;
	rtBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	rtBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	rtBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// Fill all 8 slots with the same setup unless you use independent blending
	for (int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		blendDesc.RenderTarget[i] = rtBlendDesc;
	return blendDesc;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	CommandLine::Parse(__argc, __argv);
	Render::Window window = Render::Window("Raytracer Sample", {1280, 720}, nShowCmd);
	Ref<Render::Device> device = MakeRef<Render::Device>();

	device->Init();

	//vkr::Application app;

	/////////////////////////////////////////////////////////////////////////////////////////////
	// These things should probably be encapsulated in some form of "application" class
	Ref<Render::SwapChain> swapChain = device->CreateSwapChain(window.GetNativeHandle(), { 1280, 720 });
	Graphics::ViewRenderer viewRenderer;
	if (!viewRenderer.Init(device))
		return 1;

	//Init cube resources: shaders, root signature, pso, material, mesh, model... (probably we can move all this later to the "app")
	Ref<Render::Shader> VS = device->CreateShader("../../../content/shaders/simpleforwardtestVS.hlsl", L"MainVS", vkr::Render::SHADER_STAGE_VERTEX, vkr::Render::ShaderModel::SM_6_0);
	Ref<Render::Shader> PS = device->CreateShader("../../../content/shaders/simpleforwardtestPS.hlsl", L"MainPS", vkr::Render::SHADER_STAGE_PIXEL, vkr::Render::ShaderModel::SM_6_0);
	
	Render::PipelineStateDesc psodesc;
	psodesc.m_Type = vkr::Render::PIPELINE_STATE_TYPE_DEFAULT;
	psodesc.Default.m_PrimitiveType = vkr::Render::PRIMITIVE_TYPE_TRIANGLE;
	psodesc.Default.m_VertexLayout.m_Attributes.insert({ vkr::Render::VertexAttribute::TYPE_POSITION, 0, 0, vkr::Render::FORMAT_RGB32_FLOAT });
	psodesc.Default.m_VertexShader = VS.get();
	psodesc.Default.m_PixelShader = PS.get();
	psodesc.Default.m_RasterizerState = { vkr::Render::FACE_CULL_MODE_BACK, true, false, false};
	psodesc.Default.m_RenderTargetState = { {vkr::Render::Format::FORMAT_RGBA8_UNORM_SRGB} };
	psodesc.Default.m_DepthStencilState = { true, true, vkr::Render::COMPARISON_FUNC_LESS };
	psodesc.Default.m_BlendState.m_D3DBlendDesc = CreateDefaultBlendDesc();
	Ref<vkr::Render::PipelineState> cubemainpso = device->CreatePipelineState(psodesc);
	Ref<Graphics::Material> cubematerial = MakeRef<Graphics::Material>();
	cubematerial->SetPipelineState(cubemainpso);
	Graphics::Model::Part part;
	part.m_Material = cubematerial;
	part.m_Mesh = vkr::CreateCubeMesh(device);
	Ref<Graphics::Model> cube = MakeRef<Graphics::Model>();
	cube->AddPart(part);

	Ref<Graphics::Camera> camera = MakeRef<Graphics::Camera>();
	Mat43 camtransform(Mat33::CreateRotationZ(std::numbers::pi), Vector3f(20,0,0));
	camera->SetLocalTransform(camtransform);
	camera->SetupPerspective(std::numbers::pi / 2.0f, 1280/720, 1, 2000);
	Graphics::Scene scene;
	Ref<Graphics::ModelObject> modelinst = MakeRef<Graphics::ModelObject>();
	modelinst->SetModel(cube);
	scene.AddObject(modelinst);
	Ref<Graphics::View> view = scene.CreateView();
	/////////////////////////////////////////////////////////////////////////////////////////////

	// Should we encapsulate this loop to accommodate different sample apps?
	bool running = true;
	while (running)
	{
		MSG msg = {};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				running = false;
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// app.Update();

		//////////////////////////////////////////////////////////////////////
		// These should probably also move into whatever app class we build
		//gameworld->Update()
		view->SetCamera(*camera);

		Ref<vkr::Render::Texture> backbuffer = swapChain->GetOutputTexture();
		Render::ResourceDescriptorDesc rtdesc;
		rtdesc.Type = vkr::Render::ResourceDescriptorType::RTV;
		Ref<Render::ResourceDescriptor> rtdescriptor = device->GetOrCreateDescriptor(backbuffer.get(), rtdesc);
		view->SetOutputTarget(rtdescriptor);
		scene.Update();
		scene.PrepareView(*view);
		viewRenderer.RenderView(*view);
		swapChain->Present();

		device->GarbageCollect();
		//////////////////////////////////////////////////////////////////////
	}
	return 0;
}