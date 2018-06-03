#include "MaterialSystem.h"

#include "TextureSystem.h"
#include "Lambertian.h"

//
// Data
//

static std::vector<Material *> materials{};

//
// Public API
//

void
MaterialSystem::Init()
{
	// Create the default material (ID=0)
	auto defaultMaterial = new Lambertian();
	defaultMaterial->diffuseTexture = TextureSystem::LoadLdrImage("assets/images/default.png");

	assert(materials.size() == 0);
	MaterialSystem::Add(defaultMaterial);
}

void
MaterialSystem::Destroy()
{
	for (size_t i = 0; i < materials.size(); ++i)
	{
		delete materials[i];
	}
	materials.clear();
}

int
MaterialSystem::Add(Material *material)
{
	int ID = int(materials.size());
	material->Init(ID);
	materials.emplace_back(std::move(material));
	return ID;
}

int
MaterialSystem::Create(const tinyobj::material_t& materialDescription, const std::string& baseDirectory)
{
	//
	// TODO: Create materials properly! Don't assume a certain material, do the best possible material given the supplied properties
	//
	auto material = new Lambertian();

	if (!materialDescription.diffuse_texname.empty())
	{
		material->diffuseTexture = TextureSystem::LoadLdrImage(baseDirectory + materialDescription.diffuse_texname);
	}

	return MaterialSystem::Add(material);
}

Material&
MaterialSystem::Get(int materialID)
{
	Material& material = *materials[materialID];
	return material;
}
