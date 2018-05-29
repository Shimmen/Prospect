#include "MaterialSystem.h"

#include "Lambertian.h"

//
// Data
//

std::vector<std::unique_ptr<Material>> materials{};

//
// Code
//

int
MaterialSystem::Add(std::unique_ptr<Material> material)
{
	int ID = int(materials.size());
	materials.emplace_back(std::move(material));
	return ID;
}

int
MaterialSystem::Create(const tinyobj::material_t& materialDescription)
{
	// Create the default material if it doesn't already exist
	if (materials.empty())
	{
		// TODO: Maybe this shouldn't be done here, since we shouldn't need shader stuff here.
		// Or maybe just make everything static..? I dunno. We still do need stuff like the shaderSystem right?
		// Well, really, everything should just be static right? We only every have one instance at a time. Yeah...

		//auto defaultMaterial = std::make_unique<Lambertian>();
		//defaultMaterial->diffuseTexture = textureSystem.Something();
		//defaultMaterial->Init(shaderSystem);

		//materials.emplace_back(defaultMaterial);
	}

	// TODO: Create material!
	std::unique_ptr<Material> material;
	return MaterialSystem::Add(std::move(material));
}

Material&
MaterialSystem::Get(int materialID)
{
	Material& material = *materials[materialID];
	return material;
}
