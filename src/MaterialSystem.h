#pragma once

#include <memory>

#include <tiny_obj_loader.h>

#include "Material.h"

class MaterialSystem
{
public:

	MaterialSystem() = delete;
	~MaterialSystem() = delete;

	MaterialSystem(MaterialSystem& other) = delete;
	MaterialSystem& operator=(MaterialSystem& other) = delete;

	static int Add(std::unique_ptr<Material> material);
	static int Create(const tinyobj::material_t& materialDescription);
	static Material& Get(int materialID);

private:

	static std::vector<std::unique_ptr<Material>> materials;

};
