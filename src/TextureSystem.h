#pragma once

#include <mutex>
#include <thread>
#include <condition_variable>

#include <string>
#include <memory>
#include <stdint.h>
#include <unordered_map>

#include <glad/glad.h>

#include "Queue.h"

namespace TextureSystem
{
	void Init();
	void Destroy();

	// Must be called on a regular basis (e.g. in the beginning of every frame)
	void Update();

	bool IsHdrFile(const std::string& filename);

	GLuint LoadLdrImage(const std::string& filename);
	GLuint LoadHdrImage(const std::string& filename);
	GLuint LoadDataTexture(const std::string& filename);
}
