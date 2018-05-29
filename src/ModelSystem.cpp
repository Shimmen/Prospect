#include "ModelSystem.h"

#include <tiny_obj_loader.h>

#include "mesh_attributes.h"

#include "Logging.h"
#include "MaterialSystem.h"

ModelSystem::ModelSystem(const ModelLoadCallback& onModelLoadCallback)
	: onModelLoadCallback(onModelLoadCallback)
{
	runBackgroundLoop = true;
	backgroundThread = std::thread([this]()
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

			std::string mtlBaseDirectory = "";
			if (pathIndex != std::string::npos)
			{
				mtlBaseDirectory = currentFile.substr(0, pathIndex + 1);
			}

			tinyobj::attrib_t attributes;
			std::vector<tinyobj::shape_t> shapes;
			std::vector<tinyobj::material_t> materials;

			const bool triangulate = true;
			std::string error;
			if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &error, currentFile.c_str(), mtlBaseDirectory.c_str(), triangulate))
			{
				Log("Could not load model '%s': %s.\n", currentFile.c_str(), error.c_str());
				continue;
			}

			// Register/create all materials
			std::map<int, int> materialIDs;
			for (int i = 0; i < materials.size(); ++i)
			{
				int materialID = MaterialSystem::Create(materials[i]);
				materialIDs[i] = materialID;
			}

			for (auto& shape : shapes)
			{
				auto qualifiedName = currentFile + "-" + shape.name;

				// Push model onto the map and get a local reference to it (so we never have to make a copy of it)
				loadedModels[qualifiedName] = LoadedModel{};
				LoadedModel& model = loadedModels[qualifiedName];

				model.filename = currentFile;
				model.name = shape.name;

				if (shape.mesh.material_ids.size() > 1)
				{
					Log("Mesh '%s' in '%s' has more than one material. Only the first will be used, so some things might not look as intended.", shape.name.c_str(), currentFile.c_str());
				}
				if (shape.mesh.material_ids.size() == 1)
				{
					int firstMaterialIndex = shape.mesh.material_ids[0];
					model.materialID = materialIDs[firstMaterialIndex];
				}
				else
				{
					// No material defined, so use the default material (ID=0)
					model.materialID = 0;
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

ModelSystem::~ModelSystem()
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
		glEnableVertexArrayAttrib(vao, MESH_ATTRIB_POSITION);
		glVertexArrayAttribFormat(vao, MESH_ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
		glVertexArrayAttribBinding(vao, MESH_ATTRIB_POSITION, vertexArrayBindingIndex);

		glEnableVertexArrayAttrib(vao, MESH_ATTRIB_NORMAL);
		glVertexArrayAttribFormat(vao, MESH_ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
		glVertexArrayAttribBinding(vao, MESH_ATTRIB_NORMAL, vertexArrayBindingIndex);

		glEnableVertexArrayAttrib(vao, MESH_ATTRIB_TEX_COORD);
		glVertexArrayAttribFormat(vao, MESH_ATTRIB_TEX_COORD, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texCoord));
		glVertexArrayAttribBinding(vao, MESH_ATTRIB_TEX_COORD, vertexArrayBindingIndex);

		Model model;
		model.vao = vao;
		model.indexCount = indexCount;
		model.indexType = indexType;
		model.materialID = loadedModel.materialID;

		onModelLoadCallback(model, loadedModel.filename, loadedModel.name);
	}
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
		// Since this is the line that pushes to pendingJobs it doesn't have to be locked! (The consumers are correctly locking)
		pendingJobs.Push(filename);
		runCondition.notify_all();
	}
}
