#include "MaterialSystem.h"

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
MaterialSystem::CreateMaterial(const tinyobj::material_t& materialDescription, const std::string& baseDirectory)
{
	//
	// TODO: Create materials properly! Don't assume a certain material, do the best possible material given the supplied properties
	//
	Material *material;

	bool hasDiffuseTex = !materialDescription.diffuse_texname.empty();
	bool hasNormalMap = !materialDescription.normal_texname.empty();
	bool hasRoughnessMap = !materialDescription.roughness_texname.empty();
	bool hasMetallicMap = !materialDescription.metallic_texname.empty();

	if (hasDiffuseTex && hasNormalMap && hasRoughnessMap && hasMetallicMap)
	{
		auto mat = new CompleteMaterial();

		mat->baseColorTexture = TextureSystem::LoadLdrImage(baseDirectory + materialDescription.diffuse_texname);
		mat->normalMap = TextureSystem::LoadDataTexture(baseDirectory + materialDescription.normal_texname);
		mat->roughnessMap = TextureSystem::LoadDataTexture(baseDirectory + materialDescription.roughness_texname);
		mat->metallicMap= TextureSystem::LoadDataTexture(baseDirectory + materialDescription.metallic_texname);

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
MaterialSystem::ManageMaterial(Material* material)
{
	managedMaterials.push_back(material);
}

void
MaterialSystem::Destroy()
{
	for (Material *material : managedMaterials)
	{
		delete material;
	}
}