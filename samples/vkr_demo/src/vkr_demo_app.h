#pragma once

#include "application/application.h"
#include "core/types.h"

class DemoApp : public vkr::Application
{
public:
	DemoApp();
	~DemoApp() override;

	vkr::ReturnCode InitInternal() override;
	vkr::ReturnCode TickInternal(float deltaTime) override;

private:

};