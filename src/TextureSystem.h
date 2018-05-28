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

class TextureSystem
{
public:

	TextureSystem();
	~TextureSystem();

	TextureSystem(TextureSystem& other) = delete;
	TextureSystem& operator=(TextureSystem& other) = delete;

	// Must be called on a regular basis (e.g. in the beginning of every frame)
	void Update();

	bool IsHdrFile(const std::string& filename) const;

	GLuint LoadLdrImage(const std::string& filename);
	GLuint LoadHdrImage(const std::string& filename);
	GLuint LoadDataTexture(const std::string& filename);

private:

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

	GLuint CreateEmptyTextureObject() const;
	void CreateMutableTextureFromPixel(const ImageLoadDescription& dsc, const uint8_t pixel[4]) const;
	void CreateImmutableTextureFromImage(const ImageLoadDescription& dsc, const LoadedImage& image) const;

	std::unordered_map<std::string, LoadedImage> loadedImages;
	Queue<ImageLoadDescription> pendingJobs;
	Queue<ImageLoadDescription> finishedJobs;

	std::thread             backgroundThread;
	std::mutex              accessMutex;
	std::condition_variable runCondition;
	bool                    runBackgroundLoop;

};
