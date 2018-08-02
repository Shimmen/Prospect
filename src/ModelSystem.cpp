#include "ModelSystem.h"

#include <tiny_obj_loader.h>

#include "shader_locations.h"

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

	bool materialDefined;
	std::string baseDirectory;
	tinyobj::material_t materialDescription;
};

//
// Data
//

static ModelSystem::ModelLoadCallback onModelLoadCallback;

// List of all loaded files and the models defined within them
static std::unordered_map<std::string, std::vector<std::string>> loadedFiles{};
// Cache of all loaded models. The key is the full model name (filename+shapename)
static std::unordered_map<std::string, LoadedModel> loadedModels{};

// The job string refers to the filename (e.g. teapot.obj)
static Queue<std::string> pendingJobs{};

// The job string refers to the filename+shapename (e.g. teapot.obj-lid)
static Queue<std::string> finishedJobs{};

static std::thread             backgroundThread;
static std::mutex              accessMutex;
static std::condition_variable runCondition;
static bool                    runBackgroundLoop;

//
// Public API
//

void
ModelSystem::Init()
{
	// Setup default model load callback to alert if not set
	ModelSystem::SetModelLoadCallback([](Model, const std::string& filename, const std::string& meshname) {
		Log("Model loaded in, but no model load callback is set!\n");
	});

	runBackgroundLoop = true;
	backgroundThread = std::thread([]()
	{
		while (runBackgroundLoop)
		{
			std::string currentFile;
			{
				std::unique_lock<std::mutex> lock(accessMutex);
				while (pendingJobs.IsEmpty() && runBackgroundLoop)
				{
					runCondition.wait(lock);
				}

				if (!runBackgroundLoop)
				{
					return;
				}

				currentFile = pendingJobs.Pop();
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

			for (auto& shape : shapes)
			{
				auto qualifiedName = currentFile + "-" + shape.name;

				// Push model onto the map and get a local reference to it (so we never have to make a copy of it)
				loadedModels[qualifiedName] = LoadedModel{};
				LoadedModel& model = loadedModels[qualifiedName];

				model.filename = currentFile;
				model.name = shape.name;

				int firstMaterialIndex = shape.mesh.material_ids[0];
				for (int i = 1; i < shape.mesh.material_ids.size(); ++i)
				{
					if (shape.mesh.material_ids[i] != firstMaterialIndex)
					{
						Log("Mesh '%s' in '%s' has more than one material defined. Only the first will be used, so some things might not look as intended.\n", shape.name.c_str(), currentFile.c_str());
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

				// Make sure to assign the filename to the qualified names of all the loaded models
				loadedFiles[currentFile].push_back(qualifiedName);

				{
					// Push loaded and processed data for this shape when finished so that Update() can load it to the GPU as soon as possible
					std::lock_guard<std::mutex> lock(accessMutex);
					finishedJobs.Push(qualifiedName);
				}
			}
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
	if (finishedJobs.IsEmpty())
	{
		return;
	}

	// If there definitely is at least one finished job, aqcuire a lock
	std::lock_guard<std::mutex> lock(accessMutex);

	while (!finishedJobs.IsEmpty())
	{
		std::string qualifiedName = finishedJobs.Pop();
		const LoadedModel& loadedModel = loadedModels[qualifiedName];

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
			indexType  = GL_UNSIGNED_INT;

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

		// Create a transform for the object
		// TODO: Implement some pareting system for models within files, and more. For example,
		// when calling LoadModel you get an ID for the parent transform, and all of the children
		// transforms (i.e. these ones), refer to that one as their parent.
		model.transformID = TransformSystem::Create();

		// Register/create material (must be done here on the main thread!)
		if (loadedModel.materialDefined)
		{
			model.material = MaterialFactory::CreateMaterial(loadedModel.materialDescription, loadedModel.baseDirectory);
		}

		onModelLoadCallback(std::move(model), loadedModel.filename, loadedModel.name);
	}
}

void
ModelSystem::SetModelLoadCallback(const ModelLoadCallback callback)
{
	onModelLoadCallback = callback;
}

void
ModelSystem::LoadModel(const std::string& filename)
{
	if (loadedModels.find(filename) != loadedModels.end())
	{
		// File is already loaded. Immediately push the models to the done queue so that
		// the Update() method will notice it and create GPU represenations for it
		std::lock_guard<std::mutex> lock(accessMutex);
		for (auto qualifiedModelName : loadedFiles[filename])
		{
			finishedJobs.Push(qualifiedModelName);
		}
	}
	else
	{
		// Since this is the only line that pushes to pendingJobs it doesn't have to be locked! (The consumers are correctly locking)
		// TODO: Seems as we need this lock anyway? Debug some...
		std::lock_guard<std::mutex> lock(accessMutex);
		pendingJobs.Push(filename);
		runCondition.notify_all();
	}
}
