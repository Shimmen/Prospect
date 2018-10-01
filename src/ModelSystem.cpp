#include "ModelSystem.h"

#include <limits>
#include <tiny_obj_loader.h>

#include "shader_locations.h"

#include "Maths.h"
#include "Logging.h"
#include "MaterialFactory.h"

//
// Internal data structures
//

struct Vertex
{
	float position[3]; // (x, y, z)
	float normal[3];   // (x, y, z)
	float texCoord[2]; // (u, v)

					   //float tangents[4];  // (x, y, z, w) ???
};

struct LoadedModel
{
	std::string filename;
	std::string name;

	std::vector<uint32_t> indices;
	std::vector<Vertex> vertices;

	BoundingSphere bounds;

	bool materialDefined;
	std::string baseDirectory;
	tinyobj::material_t materialDescription;
};

//
// Data
//

static Queue<std::string> pendingFiles{};
static Queue<std::string> finishedFiles{};

static std::unordered_map<std::string, std::vector<LoadedModel>> loadedData{};

// TODO/FIXME: what if we make two calls for the same file with different callbacks?!
static std::unordered_map<std::string, ModelSystem::ModelLoadCallback> callbackForFile{};


static std::thread             backgroundThread;
static std::mutex              accessMutex;
static std::condition_variable runCondition;
static bool                    runBackgroundLoop;

//
// Internal API
//

void
ReadObjShape(LoadedModel& model, tinyobj::shape_t& shape, const std::string& filename, const std::string& baseDirectory,
	tinyobj::attrib_t& attributes, std::vector<tinyobj::material_t> materials)
{
	model.filename = filename;
	model.name = shape.name;

	int firstMaterialIndex = shape.mesh.material_ids[0];
	for (int i = 1; i < shape.mesh.material_ids.size(); ++i)
	{
		if (shape.mesh.material_ids[i] != firstMaterialIndex)
		{
			Log("Mesh '%s' in '%s' has more than one material defined. Only the first will be used, "
				"so some things might not look as intended.\n", shape.name.c_str(), filename.c_str());
			break;
		}
	}

	if (firstMaterialIndex != -1)
	{
		model.materialDefined = true;
		model.baseDirectory = baseDirectory;
		model.materialDescription = materials[firstMaterialIndex];
	}
	else
	{
		model.materialDefined = false;
	}

	size_t numIndices = shape.mesh.indices.size();
	assert(numIndices % 3 == 0);
	model.indices.resize(numIndices);

	// There will be no more vertices than indices (but probably less!)
	model.vertices.reserve(numIndices);

	std::unordered_map<uint64_t, uint32_t> indexMap;

	glm::vec3 minVertex{ std::numeric_limits<float>::infinity() };
	glm::vec3 maxVertex{ -std::numeric_limits<float>::infinity() };

	for (size_t i = 0; i < numIndices; ++i)
	{
		tinyobj::index_t index = shape.mesh.indices[i];
		uint64_t hash = (index.vertex_index * 53 + index.normal_index) * 31 + index.texcoord_index;

		if (indexMap.find(hash) != indexMap.end())
		{
			// This exact vertex already exist, push index of that one
			uint32_t index = indexMap[hash];
			model.indices[i] = index;
		}
		else
		{
			// This exact vertex doesn't exist, so we have to create it

			Vertex v;

			v.position[0] = attributes.vertices[3 * index.vertex_index + 0];
			v.position[1] = attributes.vertices[3 * index.vertex_index + 1];
			v.position[2] = attributes.vertices[3 * index.vertex_index + 2];

			// Grow the bounding box around the vertex if required
			{
				minVertex.x = fmin(minVertex.x, v.position[0]);
				minVertex.y = fmin(minVertex.y, v.position[1]);
				minVertex.z = fmin(minVertex.z, v.position[2]);

				maxVertex.x = fmax(maxVertex.x, v.position[0]);
				maxVertex.y = fmax(maxVertex.y, v.position[1]);
				maxVertex.z = fmax(maxVertex.z, v.position[2]);
			}

			bool hasNormal = index.normal_index != -1;
			bool hasTexCoord = index.texcoord_index != -1;

			if (hasNormal)
			{
				v.normal[0] = attributes.normals[3 * index.normal_index + 0];
				v.normal[1] = attributes.normals[3 * index.normal_index + 1];
				v.normal[2] = attributes.normals[3 * index.normal_index + 2];
			}
			else
			{
				// TODO: Calculate normal
				v.normal[0] = 0.0f;
				v.normal[1] = 0.0f;
				v.normal[2] = 0.0f;
			}

			if (hasTexCoord)
			{
				v.texCoord[0] = attributes.texcoords[2 * index.texcoord_index + 0];
				v.texCoord[1] = attributes.texcoords[2 * index.texcoord_index + 1];
			}

			if (hasNormal && hasTexCoord)
			{
				// TODO: Calculate tangent
			}

			size_t thisIndex = model.vertices.size();
			model.vertices.emplace_back(v);

			assert(thisIndex < UINT32_MAX);
			uint32_t index = static_cast<uint32_t>(thisIndex);
			model.indices[i] = index;
			indexMap[hash] = index;
		}
	}

	// Make a bounding sphere around the mesh
	model.bounds.center = glm::mix(minVertex, maxVertex, 0.5f);
	model.bounds.radius = glm::distance(model.bounds.center, maxVertex);
}

//
// Public API
//

void
ModelSystem::Init()
{
	runBackgroundLoop = true;
	backgroundThread = std::thread([]()
	{
		while (runBackgroundLoop)
		{
			std::string currentFile;
			{
				std::unique_lock<std::mutex> lock(accessMutex);
				while (pendingFiles.IsEmpty() && runBackgroundLoop)
				{
					runCondition.wait(lock);
				}

				if (!runBackgroundLoop)
				{
					return;
				}

				currentFile = pendingFiles.Pop();
			}

			auto pathIndex = currentFile.find_last_of('/');
			if (pathIndex == std::string::npos) pathIndex = currentFile.find_last_of('\\');

			std::string baseDirectory = "";
			if (pathIndex != std::string::npos)
			{
				baseDirectory = currentFile.substr(0, pathIndex + 1);
			}

			tinyobj::attrib_t attributes;
			std::vector<tinyobj::shape_t> shapes;
			std::vector<tinyobj::material_t> materials;

			const bool triangulate = true;
			std::string error;
			if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &error, currentFile.c_str(), baseDirectory.c_str(), triangulate))
			{
				Log("Could not load model '%s': %s.\n", currentFile.c_str(), error.c_str());
				continue;
			}

			loadedData[currentFile].resize(shapes.size());

			for (int shapeIdx = 0; shapeIdx < shapes.size(); ++shapeIdx)
			{
				tinyobj::shape_t& shape = shapes[shapeIdx];
				LoadedModel& loadedModel = loadedData[currentFile][shapeIdx];

				ReadObjShape(loadedModel, shape, currentFile, baseDirectory, attributes, materials);
			}

			// Push loaded and processed data for this shape when finished so that Update() can load it to the GPU as soon as possible
			std::lock_guard<std::mutex> lock(accessMutex);
			finishedFiles.Push(currentFile);
		}
	});
}

void
ModelSystem::Destroy()
{
	// Shut down the background thread
	runBackgroundLoop = false;
	runCondition.notify_all();
	backgroundThread.join();
}

void
ModelSystem::Update()
{
	// If it's empty don't even bother to try to get a lock etc. Most frames there won't
	// be anything here, so this makes sure we don't waste precious frame time.
	if (finishedFiles.IsEmpty())
	{
		return;
	}

	// If there definitely is at least one finished job, aqcuire a lock
	std::lock_guard<std::mutex> lock(accessMutex);

	while (!finishedFiles.IsEmpty())
	{
		std::string filename = finishedFiles.Pop();
		const std::vector<LoadedModel>& loadedModels = loadedData[filename];

		std::vector<Model> models;
		models.reserve(loadedModels.size());

		for (int modelIdx = 0; modelIdx < loadedModels.size(); ++modelIdx)
		{
			const LoadedModel& loadedModel = loadedModels[modelIdx];

			if (loadedModel.indices.size() <= 0 || loadedModel.vertices.size() <= 0)
			{
				Log("Ignoring model since it has either no indices or no vertices defined!\n");
				continue;
			}

			GLuint indexBuffer;
			GLsizei indexCount;
			GLenum  indexType;
			{
				glCreateBuffers(1, &indexBuffer);

				const auto& data = loadedModel.indices;

				indexCount = static_cast<GLsizei>(data.size());
				indexType = GL_UNSIGNED_INT;

				size_t size = sizeof(uint32_t) * indexCount;

				GLbitfield flags = GL_DYNAMIC_STORAGE_BIT; // TODO: Consider these! Good default?
				glNamedBufferStorage(indexBuffer, size, data.data(), flags);
			}

			GLuint vertexBuffer;
			{
				glCreateBuffers(1, &vertexBuffer);

				const auto& data = loadedModel.vertices;
				size_t size = sizeof(Vertex) * data.size();
				GLbitfield flags = GL_DYNAMIC_STORAGE_BIT; // TODO: Consider these! Good default?
				glNamedBufferStorage(vertexBuffer, size, data.data(), flags);
			}

			GLuint vao;
			glCreateVertexArrays(1, &vao);

			// Specify the element buffer for this vertex array
			glVertexArrayElementBuffer(vao, indexBuffer);

			// Bind the vertex array to a specific binding index and specify it stride, etc.
			GLuint vertexArrayBindingIndex = 0;
			glVertexArrayVertexBuffer(vao, vertexArrayBindingIndex, vertexBuffer, 0, sizeof(Vertex));

			// Enable the attribute, specify its format, and connect the vertex array (at its
			// binding index) to to this specific attribute for this vertex array
			glEnableVertexArrayAttrib(vao, PredefinedAttributeLocation(a_position));
			glVertexArrayAttribFormat(vao, PredefinedAttributeLocation(a_position), 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
			glVertexArrayAttribBinding(vao, PredefinedAttributeLocation(a_position), vertexArrayBindingIndex);

			glEnableVertexArrayAttrib(vao, PredefinedAttributeLocation(a_normal));
			glVertexArrayAttribFormat(vao, PredefinedAttributeLocation(a_normal), 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
			glVertexArrayAttribBinding(vao, PredefinedAttributeLocation(a_normal), vertexArrayBindingIndex);

			glEnableVertexArrayAttrib(vao, PredefinedAttributeLocation(a_tex_coord));
			glVertexArrayAttribFormat(vao, PredefinedAttributeLocation(a_tex_coord), 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texCoord));
			glVertexArrayAttribBinding(vao, PredefinedAttributeLocation(a_tex_coord), vertexArrayBindingIndex);

			Model model;
			model.vao = vao;
			model.indexCount = indexCount;
			model.indexType = indexType;

			model.bounds = loadedModel.bounds;
			model.transformID = TransformSystem::Create();

			// Register/create material (must be done here on the main thread!)
			if (loadedModel.materialDefined)
			{
				model.material = MaterialFactory::CreateMaterial(loadedModel.materialDescription, loadedModel.baseDirectory);
			}

			models.emplace_back(model);
		}

		auto callback = callbackForFile[filename];
		callback(models);
	}
}

void
ModelSystem::LoadModel(const std::string& filename, const ModelLoadCallback& callback)
{
	if (loadedData.find(filename) != loadedData.end())
	{
		// File is already loaded. Immediately push the models to the done queue so that
		// the Update() method will notice it and create GPU represenations for it

		std::lock_guard<std::mutex> lock(accessMutex);

		callbackForFile[filename] = callback;
		finishedFiles.Push(filename);
	}
	else
	{
		std::lock_guard<std::mutex> lock(accessMutex);

		callbackForFile[filename] = callback;
		pendingFiles.Push(filename);

		runCondition.notify_all();
	}
}
