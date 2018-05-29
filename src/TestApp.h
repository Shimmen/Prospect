#pragma once

#include "App.h"

class TestApp: public App
{
public:

	TestApp() = default;
	virtual ~TestApp() = default;

	Settings Setup() override;

	void Init() override;
	void Resize(int width, int height) override;
	void Draw(const Input& input, float deltaTime) override;

};
