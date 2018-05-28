#pragma once

#include "App.h"

class TestApp: public App
{
public:

	TestApp() = default;
	virtual ~TestApp() = default;

	//
	// Program lifetime
	//

	Settings Setup() override;

	void Init() override;
	void Resize(int width, int height) override;
	void Draw(const Input& input, float deltaTime) override;

	//
	// Events
	//

	void OnModelLoad(Model model, const std::string& filename, const std::string& modelname) override;

};
