#pragma once

#include <mutex>
#include <thread>
#include <condition_variable>

#include <string>
#include <vector>
#include <unordered_map>

#include <glad/glad.h>
#include <tiny_obj_loader.h>

#include "Queue.h"

struct Model
{
	GLuint vao;

	GLsizei indexCount;
	GLenum  indexType;

	int transformID = 0;
	int materialID  = 0; // TODO: Implement materials! Maybe in a similar way to transforms, with IDs and all?

	void Draw() const
	{
		if (vao)
		{
			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES, indexCount, indexType, nullptr);
		}
	}
};

class ModelSystem
{
public:

	using ModelLoadCallback = std::function<void(Model, const std::string& filename, const std::string& meshname)>;

	ModelSystem(const ModelLoadCallback& onModelLoadCallback);
	~ModelSystem();

	ModelSystem(ModelSystem& other) = delete;
	ModelSystem& operator=(ModelSystem& other) = delete;

	void Update();

	void LoadModel(const std::string& filename);

private:

	ModelLoadCallback onModelLoadCallback;

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
		/*
		std::vector<float> positions; 
		std::vector<float> normals;   
		std::vector<float> tangents;  
		std::vector<float> texCoords; 
		*/
	};

	// List of all loaded files and the models defined within them
	std::unordered_map<std::string, std::vector<std::string>> loadedFiles;
	// Cache of all loaded models. The key is the full model name (filename+shapename)
	std::unordered_map<std::string, LoadedModel> loadedModels;

	// The job string refers to the filename (e.g. teapot.obj)
	Queue<std::string> pendingJobs;

	// The job string refers to the filename+shapename (e.g. teapot.obj-lid)
	Queue<std::string> finishedJobs;

	std::thread             backgroundThread;
	std::mutex              accessMutex;
	std::condition_variable runCondition;
	bool                    runBackgroundLoop;

};
