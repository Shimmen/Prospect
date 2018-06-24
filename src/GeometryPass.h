#pragma once

#include <vector>

#include "Model.h"
#include "FpsCamera.h"

class GeometryPass
{
public:

	// TODO: Maybe also pass in some GBuffer object? Or not..?
	void Draw(const std::vector<Model>& opaqueGeometry, FpsCamera& camera);

};
