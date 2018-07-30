#include "MaterialFactory.h"

#include "TextureSystem.h"

// Materials
#include <Lambertian.h>

//
// Data
//

static std::vector<Material *> managedMaterials{};

//
// Public API
//

Material *
MaterialFactory::CreateMaterial(const tinyobj::material_t& materialDescription, const std::string& baseDirectory)
{
	//
	// TODO: Create materials properly! Don't assume a certain material, do the best possible material given the supplied properties
	//
	auto material = new Lambertian();

	if (!materialDescription.diffuse_texname.empty())
	{
		material->diffuseTexture = TextureSystem::LoadLdrImage(baseDirectory + materialDescription.diffuse_texname);
	}

	managedMaterials.push_back(material);
	return material;
}

void
MaterialFactory::DeleteManagedMaterials()
{
	for (Material *material : managedMaterials)
	{
		delete material;
	}
}