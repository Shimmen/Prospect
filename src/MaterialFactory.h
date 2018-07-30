#pragma once

#include <memory>

#include <tiny_obj_loader.h>

#include "Material.h"

namespace MaterialFactory
{
	Material *CreateMaterial(const tinyobj::material_t& materialDescription, const std::string& baseDirectory);

	void DeleteManagedMaterials();
}
