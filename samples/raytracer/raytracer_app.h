#pragma once

#include "application/application.h"
#include "core/types.h"
#include "tickable.h"

class RaytracerApp : public vkr::Application 
{
public:
	RaytracerApp();
	~RaytracerApp() override;

	void AppInit() override;
	void Tick(float deltaTime) override;

private:
};