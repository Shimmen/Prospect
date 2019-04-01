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

	bool IsIdle();

	bool IsHdrFile(const std::string& filename);

	GLuint CreatePlaceholder(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF);

	GLuint CreateTexture(int width, int height, GLenum format,
		GLenum minFilter = GL_LINEAR_MIPMAP_LINEAR, GLenum magFilter = GL_LINEAR);

	GLuint LoadLdrImage(const std::string& filename);
	GLuint LoadHdrImage(const std::string& filename);
	GLuint LoadDataTexture(const std::string& filename);
}
