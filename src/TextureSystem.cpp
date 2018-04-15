#include "TextureSystem.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Logging.h"

TextureSystem::TextureSystem()
{
	// Basic setup
	stbi_set_flip_vertically_on_load(true);
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &textureMaxAnisotropy);

	// Start the background thread for loading
	runBackgroundLoop = true;
	backgroundThread = std::thread([this]()
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

			if (currentJob.isHdr)
			{
				image.pixels = stbi_loadf(filename, &image.width, &image.height, nullptr, STBI_rgb);
				if (!image.pixels)
				{
					Log("Could not load HDR image '%s': %s.\n", filename, stbi_failure_reason());
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
					continue;
				}
				image.type = GL_UNSIGNED_BYTE;
			}

			// We should never load an image if it's already loaded, but this can happen if we quickly
			// call some LoadImage function a second time before the first image has finished loading.
			if (loadedImages.find(filename) != loadedImages.end())
			{
				stbi_image_free(image.pixels);
			}
			else
			{
				loadedImages[filename] = image;
			}

			{
				std::lock_guard<std::mutex> lock(accessMutex);
				finishedJobs.Push(currentJob);
			}
		}
	});
}

TextureSystem::~TextureSystem()
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
	}
}

bool
TextureSystem::IsHdrFile(const std::string& filename) const
{
	return stbi_is_hdr(filename.c_str());
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
	dsc.shouldGenerateMipmaps = true;
	dsc.isHdr = false;

	if (loadedImages.find(filename) != loadedImages.end())
	{
		// The file is already loaded into memory, just fill in the GPU texture data
		const LoadedImage& image = loadedImages[filename];
		CreateImmutableTextureFromImage(dsc, image);
	}
	else
	{
		// Fill texture with placeholder data and request an image load
		static uint8_t placeholderImageData[4] = { 128, 128, 128, 255 };
		CreateMutableTextureFromPixel(dsc, placeholderImageData);

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
	dsc.shouldGenerateMipmaps = true;
	dsc.isHdr = true;

	if (loadedImages.find(filename) != loadedImages.end())
	{
		const LoadedImage& image = loadedImages[filename];
		CreateImmutableTextureFromImage(dsc, image);
	}
	else
	{
		static uint8_t placeholderImageData[4] = { 128, 128, 128, 255 };
		CreateMutableTextureFromPixel(dsc, placeholderImageData);

		pendingJobs.Push(dsc);
		runCondition.notify_all();
	}

	return dsc.texture;
}

GLuint
TextureSystem::LoadDataTexture(const std::string& filename)
{
	if (IsHdrFile(filename))
	{
		Log("Texture file '%s' is an HDR image and must be loaded as such\n", filename.c_str());
	}

	ImageLoadDescription dsc;
	dsc.filename = filename;
	dsc.texture = CreateEmptyTextureObject();
	dsc.format = GL_RGB;
	dsc.internalFormat = GL_RGB8;
	dsc.shouldGenerateMipmaps = true;
	dsc.isHdr = false;

	if (loadedImages.find(filename) != loadedImages.end())
	{
		const LoadedImage& image = loadedImages[filename];
		CreateImmutableTextureFromImage(dsc, image);
	}
	else
	{
		static uint8_t placeholderImageData[4] = { 128, 128, 128, 255 };
		CreateMutableTextureFromPixel(dsc, placeholderImageData);

		pendingJobs.Push(dsc);
		runCondition.notify_all();
	}

	return dsc.texture;
}

GLuint
TextureSystem::CreateEmptyTextureObject() const
{
	GLuint texture;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture);

	// Set some defaults (min-filter is required)
	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameterf(texture, GL_TEXTURE_MAX_ANISOTROPY, textureMaxAnisotropy);

	return texture;
}

void
TextureSystem::CreateMutableTextureFromPixel(const ImageLoadDescription& dsc, const uint8_t pixel[4]) const
{
	// Note that we can't use the bindless API for this since we need to resize the texture later,
	// which wouldn't be possible because that API only creates immutable textures (i.e. can't be resized)

	GLint lastBoundTexture2D;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastBoundTexture2D);
	{
		glBindTexture(GL_TEXTURE_2D, dsc.texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
	}
	glBindTexture(GL_TEXTURE_2D, lastBoundTexture2D);
}

void
TextureSystem::CreateImmutableTextureFromImage(const ImageLoadDescription& dsc, const LoadedImage& image) const
{
	if (dsc.shouldGenerateMipmaps)
	{
		if (image.width != image.height)
		{
			Log("Can't generate mipmaps for a non square image '%s'\n", dsc.filename.c_str());
			return;
		}

		int size = image.width;

		if ((size & (size - 1)) != 0)
		{
			Log("Can't generate mipmaps for a non power-of-two sized image '%s'\n", dsc.filename.c_str());
			return;
		}

		int numLevels = 1 + int(std::log2(size));
		glTextureStorage2D(dsc.texture, numLevels, dsc.internalFormat, image.width, image.height);
		glTextureSubImage2D(dsc.texture, 0, 0, 0, image.width, image.height, dsc.format, image.type, image.pixels);
		glGenerateTextureMipmap(dsc.texture);
	}
	else
	{
		glTextureStorage2D(dsc.texture, 1, dsc.internalFormat, image.width, image.height);
		glTextureSubImage2D(dsc.texture, 0, 0, 0, image.width, image.height, dsc.format, image.type, image.pixels);
	}
}
