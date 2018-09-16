#pragma once

#include "App.h"

class IBLDemo : public App
{
public:

	IBLDemo() = default;
	virtual ~IBLDemo() = default;

	Settings Setup() override;

	void Init() override;
	void Resize(int width, int height) override;
	void Draw(const Input& input, float deltaTime, float runningTime) override;

};
