#pragma once

#include <memory>

#include <tiny_obj_loader.h>

#include "Material.h"

namespace MaterialSystem
{
	void Init();
	void Destroy();

	int Add(Material *material);
	int Create(const tinyobj::material_t& materialDescription, const std::string& baseDirectory);

	Material& Get(int materialID);
}
