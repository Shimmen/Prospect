#pragma once

#include "App.h"

class PointcloudExplorer : public App
{
public:

	PointcloudExplorer() = default;
	virtual ~PointcloudExplorer() = default;

	Settings Setup() override;

	void Init() override;
	void Resize(int width, int height) override;
	void Draw(const Input& input, float deltaTime, float runningTime) override;

};
