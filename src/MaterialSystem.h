#pragma once

#include <memory>

#include <tiny_obj_loader.h>

#include "Material.h"

namespace MaterialSystem
{
	int Add(std::unique_ptr<Material> material);
	int Create(const tinyobj::material_t& materialDescription);

	Material& Get(int materialID);
}
