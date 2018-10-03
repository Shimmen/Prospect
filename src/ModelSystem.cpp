#include "ModelSystem.h"

#include <limits>
#include <glm/glm.hpp>
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
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec4 tangent; // (w is bitangent's handedness)
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
	const tinyobj::attrib_t& attributes, const std::vector<tinyobj::material_t> materials)
{
	model.filename = filename;
	model.name = shape.name;

	int firstMaterialIndex = shape.mesh.material_ids[0];
	for (int i = 1; i < shape.mesh.material_ids.size(); ++i)
	{
		if (shape.mesh.material_ids[i] != firstMaterialIndex)
		{
			Log("Mesh shape '%s' in '%s' has more than one material defined. Please export mesh with separate material groups! "
				"Only the first will be used, so some things might not look as intended.\n", shape.name.c_str(), filename.c_str());
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

	size_t numInputIndices = shape.mesh.indices.size();
	assert(numInputIndices % 3 == 0);

	// The number of indices will not change
	model.indices.reserve(numInputIndices);
	// There will be no more vertices than indices (but probably and hopefully less!)
	model.vertices.reserve(numInputIndices);

	int numMissingNormals = 0;
	int numMissingTexCoords = 0;

	glm::vec3 minVertex{ INFINITY };
	glm::vec3 maxVertex{ -INFINITY };

	std::unordered_map<uint64_t, uint32_t> indexMap;

	for (size_t i = 0; i < numInputIndices; ++i)
	{
		tinyobj::index_t index = shape.mesh.indices[i];
		uint64_t hash = (index.vertex_index * 53ULL + index.normal_index) * 37ULL + index.texcoord_index;

		if (indexMap.find(hash) != indexMap.end())
		{
			// This exact vertex already exist, push index of that one
			uint32_t index = indexMap[hash];
			model.indices.emplace_back(index);
		}
		else
		{
			// This exact vertex doesn't exist, so we have to create it

			Vertex v;

			v.position.x = attributes.vertices[3 * index.vertex_index + 0];
			v.position.y = attributes.vertices[3 * index.vertex_index + 1];
			v.position.z = attributes.vertices[3 * index.vertex_index + 2];

			if (index.normal_index != -1)
			{
				v.normal.x = attributes.normals[3 * index.normal_index + 0];
				v.normal.y = attributes.normals[3 * index.normal_index + 1];
				v.normal.z = attributes.normals[3 * index.normal_index + 2];
			}
			else
			{
				numMissingNormals += 1;
			}

			if (index.texcoord_index != -1)
			{
				v.texCoord.s = attributes.texcoords[2 * index.texcoord_index + 0];
				v.texCoord.t = attributes.texcoords[2 * index.texcoord_index + 1];
			}
			else
			{
				numMissingTexCoords += 1;
			}

			v.tangent = { 0, 0, 0, 0 };

			// Grow the bounding box around the vertex if required
			{
				minVertex.x = fmin(minVertex.x, v.position[0]);
				minVertex.y = fmin(minVertex.y, v.position[1]);
				minVertex.z = fmin(minVertex.z, v.position[2]);

				maxVertex.x = fmax(maxVertex.x, v.position[0]);
				maxVertex.y = fmax(maxVertex.y, v.position[1]);
				maxVertex.z = fmax(maxVertex.z, v.position[2]);
			}

			size_t thisIndex = model.vertices.size();
			model.vertices.emplace_back(v);

			assert(thisIndex < UINT32_MAX);
			uint32_t index = static_cast<uint32_t>(thisIndex);
			model.indices.emplace_back(index);
			indexMap[hash] = index;
		}
	}

	// Make a bounding sphere around the mesh
	model.bounds.center = glm::mix(minVertex, maxVertex, 0.5f);
	model.bounds.radius = glm::distance(model.bounds.center, maxVertex);

	// Generate normals (if not already exists) and tangents (if possible)
	assert(model.indices.size() % 3 == 0);

	bool generateNewNormals = numMissingNormals > 0;

	// Reset normals if we need to create new ones
	if (generateNewNormals)
	{
		for (Vertex& vertex : model.vertices)
		{
			vertex.normal = { 0, 0, 0 };
		}
	}

	bool hasTexCoords = numMissingTexCoords == 0;
	bool generateTangents = hasTexCoords;

	size_t numBitangents = (generateTangents) ? model.vertices.size() : 0;
	std::vector<glm::vec3> bitangents{ numBitangents };

	// Construct tangents (if possible) and new normals (if requested)
	for (size_t i = 0; i < model.indices.size(); i += 3)
	{
		uint32_t i0 = model.indices[i + 0];
		uint32_t i1 = model.indices[i + 1];
		uint32_t i2 = model.indices[i + 2];

		Vertex& v0 = model.vertices[i0];
		Vertex& v1 = model.vertices[i1];
		Vertex& v2 = model.vertices[i2];

		glm::vec3 e1 = v1.position - v0.position;
		glm::vec3 e2 = v2.position - v0.position;

		if (generateNewNormals)
		{
			glm::vec3 normal = glm::cross(e1, e2);
			v0.normal += normal;
			v1.normal += normal;
			v2.normal += normal;
		}

		if (generateTangents)
		{
			glm::vec2 tex1 = v1.texCoord - v0.texCoord;
			glm::vec2 tex2 = v2.texCoord - v0.texCoord;

			float r = 1.0F / (tex1.s * tex2.t - tex2.s * tex1.t);

			glm::vec3 sDir = {
				(tex2.t * e1.x - tex1.t * e2.x) * r,
				(tex2.t * e1.y - tex1.t * e2.y) * r,
				(tex2.t * e1.z - tex1.t * e2.z) * r
			};
			glm::vec3 tDir = {
				(tex1.s * e2.x - tex2.s * e1.x) * r,
				(tex1.s * e2.y - tex2.s * e1.y) * r,
				(tex1.s * e2.z - tex2.s * e1.z) * r
			};

			v0.tangent += glm::vec4(sDir, 0.0f);
			v1.tangent += glm::vec4(sDir, 0.0f);
			v2.tangent += glm::vec4(sDir, 0.0f);

			// Save bintangent temporarily
			bitangents[i0] += tDir;
			bitangents[i1] += tDir;
			bitangents[i2] += tDir;
		}
	}

	// Normalize new normals and tangents, and set handedness of (bi)tangents
	for (uint32_t i : model.indices)
	{
		Vertex& vertex = model.vertices[i];

		if (generateNewNormals)
		{
			vertex.normal = glm::normalize(vertex.normal);
		}
		
		if (generateTangents)
		{
			glm::vec3 N = vertex.normal;
			glm::vec3 B = bitangents[i];
			glm::vec3 T = glm::vec3(vertex.tangent);
			T = glm::normalize(T - N * glm::dot(N, T));

			// Calculate handedness and store in w component
			float handedness = (glm::dot(glm::cross(N, T), B) < 0.0f) ? -1.0f : 1.0f;
			vertex.tangent = glm::vec4(T, handedness);
		}
	}
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

			glEnableVertexArrayAttrib(vao, PredefinedAttributeLocation(a_tangent));
			glVertexArrayAttribFormat(vao, PredefinedAttributeLocation(a_tangent), 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, tangent));
			glVertexArrayAttribBinding(vao, PredefinedAttributeLocation(a_tangent), vertexArrayBindingIndex);

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
