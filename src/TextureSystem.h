#pragma once

#include <mutex>
#include <thread>

#include <string>
#include <cstdint>
#include <unordered_map>

#include <glad/glad.h>

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
		GLenum minFilter = GL_LINEAR_MIPMAP_LINEAR, GLenum magFilter = GL_LINEAR, bool useMips = true);

	GLuint LoadLdrImage(const std::string& filename);
	GLuint LoadHdrImage(const std::string& filename);
	GLuint LoadDataTexture(const std::string& filename, GLenum internalFormat = GL_RGBA8);
}
