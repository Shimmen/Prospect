#include "TextureSystem.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <atomic>

#include "Logging.h"
#include "Queue.h"

//
// Internal data structures
//

struct ImageLoadDescription
{
	std::string filename;
	GLuint texture;
	GLenum format, internalFormat;
	bool requestMipmaps;
	bool isHdr;
};

struct LoadedImage
{
	void* pixels;
	GLenum type;
	int width, height;
};

//
// Data
//

static std::unordered_map<std::string, LoadedImage> loadedImages{};
static Queue<ImageLoadDescription> pendingJobs{};
static Queue<ImageLoadDescription> finishedJobs{};

static std::atomic_int currentJobsCounter;

static std::thread             backgroundThread;
static std::mutex              accessMutex;
static std::condition_variable runCondition;
static bool                    runBackgroundLoop;

//
// Internal API
//

GLuint
CreateEmptyTextureObject()
{
	GLuint texture;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture);

	// Set some defaults (min-filter is required)
	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Set max anisotropy to largest supported value
	static GLfloat textureMaxAnisotropy = -1.0f;
	if (textureMaxAnisotropy == -1.0f)
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &textureMaxAnisotropy);
	}
	glTextureParameterf(texture, GL_TEXTURE_MAX_ANISOTROPY, textureMaxAnisotropy);

	return texture;
}

void
CreateMutableTextureFromPixel(GLuint texture, const uint8_t pixel[4])
{
	// Note that we can't use the bindless API for this since we need to resize the texture later,
	// which wouldn't be possible because that API only creates immutable textures (i.e. can't be resized)

	GLint lastBoundTexture2D;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastBoundTexture2D);
	{
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
	}
	glBindTexture(GL_TEXTURE_2D, lastBoundTexture2D);
}

void
CreateImmutableTextureFromImage(const ImageLoadDescription& dsc, const LoadedImage& image)
{
	// It's possible that mipmaps were requested, but now when we know the size we see that's not possible
	bool sameWidthAsHeight = image.width == image.height;
	bool powerOfTwoSize = (image.width & (image.width - 1)) == 0;
	bool generateMipmaps = dsc.requestMipmaps && sameWidthAsHeight && powerOfTwoSize;

	if (generateMipmaps)
	{
		int size = image.width;
		int numLevels = 1 + int(std::log2(size));

		glTextureParameteri(dsc.texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(dsc.texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureStorage2D(dsc.texture, numLevels, dsc.internalFormat, image.width, image.height);
		glTextureSubImage2D(dsc.texture, 0, 0, 0, image.width, image.height, dsc.format, image.type, image.pixels);
		glGenerateTextureMipmap(dsc.texture);
	}
	else
	{
		glTextureParameteri(dsc.texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(dsc.texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureStorage2D(dsc.texture, 1, dsc.internalFormat, image.width, image.height);
		glTextureSubImage2D(dsc.texture, 0, 0, 0, image.width, image.height, dsc.format, image.type, image.pixels);
	}
}

//
// Public API
//

void
TextureSystem::Init()
{
	// Basic setup
	stbi_set_flip_vertically_on_load(true);

	// Start the background thread for loading
	runBackgroundLoop = true;
	backgroundThread = std::thread([]()
	{
		while (runBackgroundLoop)
		{
			ImageLoadDescription currentJob;

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

				currentJob = pendingJobs.Pop();
			}

			const char* filename = currentJob.filename.c_str();
			LoadedImage image;

			// NOTE: If we add more threads for image loading, this check needs to be more rigorous!
			// We should never load an image if it's already loaded, but this can happen if we quickly
			// call some LoadImage function a second time before the first image has finished loading.
			if (loadedImages.find(filename) != loadedImages.end())
			{
				std::lock_guard<std::mutex> lock(accessMutex);
				finishedJobs.Push(currentJob);
				continue;
			}

			if (currentJob.isHdr)
			{
				image.pixels = stbi_loadf(filename, &image.width, &image.height, nullptr, STBI_rgb);
				if (!image.pixels)
				{
					Log("Could not load HDR image '%s': %s.\n", filename, stbi_failure_reason());
					currentJobsCounter -= 1;
					continue;
				}
				image.type = GL_FLOAT;
			}
			else
			{
				image.pixels = stbi_load(filename, &image.width, &image.height, nullptr, STBI_rgb_alpha);
				if (!image.pixels)
				{
					Log("Could not load image '%s': %s.\n", filename, stbi_failure_reason());
					currentJobsCounter -= 1;
					continue;
				}
				image.type = GL_UNSIGNED_BYTE;
			}

			loadedImages[filename] = image;

			std::lock_guard<std::mutex> lock(accessMutex);
			finishedJobs.Push(currentJob);
		}
	});
}

void
TextureSystem::Destroy()
{
	// Shut down the background thread
	runBackgroundLoop = false;
	runCondition.notify_all();
	backgroundThread.join();

	// Release all loaded images (but NOT textures!)
	for (auto& nameImagePair : loadedImages)
	{
		stbi_image_free(nameImagePair.second.pixels);
	}
}

void
TextureSystem::Update()
{
	// This is the only place that consumes finished jobs, so a check like this should work fine and be thread safe. It might be possibile that 
	// another thread will push a job and this thread doesn't notice it until later. That is okay, though, since this is called every frame.
	while (!finishedJobs.IsEmpty())
	{
		ImageLoadDescription job = finishedJobs.Pop();
		const LoadedImage& image = loadedImages[job.filename];
		CreateImmutableTextureFromImage(job, image);
		currentJobsCounter -= 1;
	}
}

bool
TextureSystem::IsIdle()
{
	assert(currentJobsCounter >= 0);
	return currentJobsCounter == 0;
}

bool
TextureSystem::IsHdrFile(const std::string& filename)
{
	return stbi_is_hdr(filename.c_str());
}

GLuint
TextureSystem::CreatePlaceholder(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	GLuint texture = CreateEmptyTextureObject();

	uint8_t pixel[4] = { r, g, b, a };
	CreateMutableTextureFromPixel(texture, pixel);

	return texture;
}

GLuint
TextureSystem::CreateTexture(int width, int height, GLenum format, GLenum minFilter, GLenum magFilter)
{
	GLuint texture;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture);
	glTextureStorage2D(texture, 1, format, width, height);

	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, minFilter);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, magFilter);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return texture;
}

GLuint
TextureSystem::LoadLdrImage(const std::string& filename)
{
	if (IsHdrFile(filename))
	{
		Log("Texture file '%s' is an HDR image and must be loaded as such\n", filename.c_str());
	}

	ImageLoadDescription dsc;
	dsc.filename = filename;
	dsc.texture = CreateEmptyTextureObject();
	dsc.format = GL_RGBA;
	dsc.internalFormat = GL_SRGB8_ALPHA8;
	dsc.requestMipmaps = true;
	dsc.isHdr = false;

	if (loadedImages.find(filename) != loadedImages.end())
	{
		// The file is already loaded into memory, just fill in the GPU texture data
		const LoadedImage& image = loadedImages[filename];
		CreateImmutableTextureFromImage(dsc, image);
	}
	else
	{
		currentJobsCounter += 1;

		// Fill texture with placeholder data and request an image load
		static uint8_t placeholderImageData[4] = { 200, 200, 200, 255 };
		CreateMutableTextureFromPixel(dsc.texture, placeholderImageData);

		pendingJobs.Push(dsc);
		runCondition.notify_all();
	}

	return dsc.texture;
}

GLuint
TextureSystem::LoadHdrImage(const std::string& filename)
{
	if (!IsHdrFile(filename))
	{
		Log("Texture file '%s' is an LDR image and must be loaded as such\n", filename.c_str());
	}

	ImageLoadDescription dsc;
	dsc.filename = filename;
	dsc.texture = CreateEmptyTextureObject();
	dsc.format = GL_RGB;
	dsc.internalFormat = GL_RGB32F; // TODO: alpha? 16b?
	dsc.requestMipmaps = true;
	dsc.isHdr = true;

	if (loadedImages.find(filename) != loadedImages.end())
	{
		const LoadedImage& image = loadedImages[filename];
		CreateImmutableTextureFromImage(dsc, image);
	}
	else
	{
		currentJobsCounter += 1;

		static uint8_t placeholderImageData[4] = { 128, 128, 128, 255 };
		CreateMutableTextureFromPixel(dsc.texture, placeholderImageData);

		pendingJobs.Push(dsc);
		runCondition.notify_all();
	}

	return dsc.texture;
}

GLuint
TextureSystem::LoadDataTexture(const std::string& filename, GLenum internalFormat)
{
	if (IsHdrFile(filename))
	{
		Log("Texture file '%s' is an HDR image and must be loaded as such\n", filename.c_str());
	}

	ImageLoadDescription dsc;
	dsc.filename = filename;
	dsc.texture = CreateEmptyTextureObject();
	dsc.format = GL_RGBA; // (we currently always try to load an RGBA)
	dsc.internalFormat = internalFormat;
	dsc.requestMipmaps = true;
	dsc.isHdr = false;

	if (loadedImages.find(filename) != loadedImages.end())
	{
		const LoadedImage& image = loadedImages[filename];
		CreateImmutableTextureFromImage(dsc, image);
	}
	else
	{
		currentJobsCounter += 1;

		static uint8_t placeholderImageData[4] = { 128, 128, 128, 255 };
		CreateMutableTextureFromPixel(dsc.texture, placeholderImageData);

		pendingJobs.Push(dsc);
		runCondition.notify_all();
	}

	return dsc.texture;
}
