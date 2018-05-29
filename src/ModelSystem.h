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
	int materialID  = 0;

	void Draw() const
	{
		if (vao)
		{
			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES, indexCount, indexType, nullptr);
		}
	}
};

namespace ModelSystem
{
	void Init();
	void Destroy();

	void Update();

	using ModelLoadCallback = std::function<void(Model, const std::string& filename, const std::string& meshname)>;
	void SetModelLoadCallback(const ModelLoadCallback onModelLoadCallback);

	void LoadModel(const std::string& filename);
}
