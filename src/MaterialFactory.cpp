#include "MaterialFactory.h"

#include "TextureSystem.h"

// Materials
#include "BasicMaterial.h"
#include "CompleteMaterial.h"

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
	Material *material;

	if (!materialDescription.diffuse_texname.empty() && !materialDescription.normal_texname.empty())
	{
		auto mat = new CompleteMaterial();

		mat->baseColor = TextureSystem::LoadLdrImage(baseDirectory + materialDescription.diffuse_texname);
		mat->normalMap = TextureSystem::LoadDataTexture(baseDirectory + materialDescription.normal_texname);

		material = mat;
	}
	else
	{
		auto mat = new BasicMaterial();

		mat->baseColor = glm::vec3(
			materialDescription.diffuse[0],
			materialDescription.diffuse[1],
			materialDescription.diffuse[2]
		);
		mat->roughness = 0.5f;
		mat->metallic = 0.0f;

		material = mat;
	}

	managedMaterials.push_back(material);
	return material;
}

void
MaterialFactory::ManageMaterial(Material* material)
{
	managedMaterials.push_back(material);
}

void
MaterialFactory::DeleteManagedMaterials()
{
	for (Material *material : managedMaterials)
	{
		delete material;
	}
}