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
#include "Model.h"

namespace ModelSystem
{
	void Init();
	void Destroy();

	void Update();

	bool IsIdle();

	using ModelLoadCallback = std::function<void(std::vector<Model> models)>;
	void LoadModel(const std::string& filename, const ModelLoadCallback& callback);
}
