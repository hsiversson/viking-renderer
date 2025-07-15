#include "raytracer_app.h"

#include "cameracontroller.h"

#include "graphics/camera.h"
#include "graphics/model.h"
#include "graphics/modelloader_gltf.h"
#include "graphics/modelobject.h"
#include "graphics/scene.h"
#include "graphics/view.h"


using namespace vkr;

void RaytracerApp::AppInit()
{
	m_RenderDevice->BeginFrame();
	Graphics::ModelLoader_GLTF loader;
	Ref<Graphics::Model> model;
	model = loader.Load("../../../content/models/cp_noodles/scene.gltf");
	Ref<Graphics::ModelObject> modelinst = MakeRef<Graphics::ModelObject>();
	modelinst->SetLocalTransform(Compose(Mat33::Identity(), Vector3f(0.0f, 0.0f, 0.0f)));

	modelinst->SetModel(model);
	m_Scene->AddObject(modelinst);

	m_Camera = MakeRef<Graphics::Camera>();
	Mat43 camtransform = Compose(Mat33::Identity(), Vector3f(0, 2.0f, -4.0f));
	m_Camera->SetLocalTransform(camtransform);
	m_Camera->SetupPerspective(std::numbers::pi / 2.0f, (float)m_WindowSize.x / (float)m_WindowSize.y, 0.1f, 1000.0f);
	m_RenderDevice->EndFrame();

	auto camController = MakeRef<CameraController>();
	camController->Init(m_Camera, m_InputManager.get());
	m_Tickables.push_back(camController);
}

void RaytracerApp::Tick(float deltaTime)
{
	//Update camera based on input
	for (auto tickable : m_Tickables)
		tickable->Tick(deltaTime);

	m_View->SetCamera(*m_Camera);
}

