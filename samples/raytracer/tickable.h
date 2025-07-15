#pragma once

struct ITickable
{
	virtual void Tick(float deltaTime) = 0;
};