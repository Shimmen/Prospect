#pragma once

#include <memory>

#include <tiny_obj_loader.h>

#include "Material.h"

namespace MaterialSystem
{
	Material *CreateMaterial(const tinyobj::material_t& materialDescription, const std::string& baseDirectory);

	void ManageMaterial(Material* material);

	void Destroy();

}
