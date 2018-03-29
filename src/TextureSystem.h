#pragma once

#include <mutex>
#include <thread>
#include <condition_variable>

#include <queue>
#include <string>
#include <memory>
#include <stdint.h>
#include <unordered_map>

#include <glad/glad.h>

class TextureSystem
{
public:

	TextureSystem();
	~TextureSystem();

	TextureSystem(TextureSystem& other) = delete;
	TextureSystem& operator=(TextureSystem& other) = delete;

	void Update();

	GLuint LoadLdrImage(const std::string& filename);
	GLuint LoadHdrImage(const std::string& filename);
	GLuint LoadDataTexture(const std::string& filename);

private:

	GLuint CreateTexture() const;

	struct ImageLoadDescription
	{
		std::string filename;
		GLuint texture;
		GLenum format, internalFormat;
		bool shouldGenerateMipmaps;
		bool isHdr;
	};

	struct LoadedImage
	{
		void* pixels;
		GLenum type;
		int width, height;
	};

	GLfloat textureMaxAnisotropy;

	std::unordered_map<std::string, LoadedImage> loadedImages;

	std::queue<ImageLoadDescription> pendingJobs;
	std::queue<ImageLoadDescription> finishedJobs;

	std::thread             backgroundThread;
	std::mutex              accessMutex;
	std::condition_variable runCondition;
	volatile bool           runBackgroundLoop;

};
