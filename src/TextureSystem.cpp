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
				while (pendingJobs.empty() && runBackgroundLoop)      // NOTE: This won't really allow any addition of new jobs until old ones are finished ... which, yeah.. *UNLESS* we never aquire the lock for the pendingJobs producer!
				{
					runCondition.wait(lock);
				}

				if (!runBackgroundLoop)
				{
					return;
				}

				currentJob = pendingJobs.front();
				pendingJobs.pop();
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
			// call some LoadImage function a second time before the first image has loaded.
			if (loadedImages.find(filename) != loadedImages.end())
			{
				stbi_image_free(image.pixels);
			}
			else
			{
				loadedImages[filename] = image;
			}

			finishedJobs.push(currentJob);
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
	//
	// TODO: Make sure that everything is thread safe!!! Don't think it is at all now.
	//       ALSO, this function has to be fast since it executes every frame. It doesn't
	//       have to block or be "precise" in that it's always up to date and so. I'm not
	//       sure what I'm talking about right now...
	//

	// This is the only place that consumes finished jobs, so a conservative check like this should work fine
	while (!finishedJobs.empty())
	{
		ImageLoadDescription job = finishedJobs.front();
		finishedJobs.pop();

		// Read for info about immutable texture storage and generating mipmaps:
		//  https://stackoverflow.com/questions/15405869/is-gltexstorage2d-imperative-when-auto-generating-mipmaps

		//
		// TODO: It seems awfully arbitrary which properties belong to job and which belong to image. Clean up maybe?
		//

		LoadedImage image = loadedImages[job.filename];

		if (job.shouldGenerateMipmaps)
		{
			if (image.width != image.height)
			{
				Log("Can't generate mipmaps for a non square image '%s'\n", job.filename.c_str());
				continue;
			}

			int size = image.width;

			if ((size & (size - 1)) != 0)
			{
				Log("Can't generate mipmaps for a non power-of-two sized image '%s'\n", job.filename.c_str());
				continue;
			}
			
			int numLevels = 1 + int(std::log2(size));
			glTextureStorage2D(job.texture, numLevels, job.internalFormat, image.width, image.height);
			glTextureSubImage2D(job.texture, 0, 0, 0, image.width, image.height, job.format, image.type, image.pixels);
			glGenerateTextureMipmap(job.texture);
		}
		else
		{
			glTextureStorage2D(job.texture, 1, job.internalFormat, image.width, image.height);
			glTextureSubImage2D(job.texture, 0, 0, 0, image.width, image.height, job.format, image.type, image.pixels);
		}
	}
}

GLuint
TextureSystem::LoadLdrImage(const std::string& filename)
{
	static uint8_t placeholderImageData[4] = { 128, 128, 128, 255 };

	if (stbi_is_hdr(filename.c_str()))
	{
		Log("Texture file '%s' is an HDR image and must be loaded as such\n", filename.c_str());
	}

	GLuint texture = CreateTexture();

	if (loadedImages.find(filename) != loadedImages.end())
	{
		// TODO: What about the format etc.? Assumed to be this? Am I too tired to think properly? Well, yes, assuming that this LoadImage call is valid, i.e. the image is an sRGB_A (not HDR is already checked above)

		LoadedImage image = loadedImages[filename];

		// TODO: Use bindless here! As above.. But probably don't copy all that over, make some convenience function
		glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.pixels);
		glGenerateTextureMipmap(texture);
	}
	else
	{
		// Fill texture with placeholder data and request an image load
		// TODO: Use bindless if possible! Not sure if possible though since the bindless stuff all seem to be immutable
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, placeholderImageData);

		ImageLoadDescription dsc;
		dsc.filename = filename;
		dsc.texture = texture;
		dsc.format = GL_RGBA;
		dsc.internalFormat = GL_SRGB8_ALPHA8;
		dsc.shouldGenerateMipmaps = true;
		dsc.isHdr = false;

		pendingJobs.push(dsc);
		runCondition.notify_all();
	}

	return texture;
}

GLuint
TextureSystem::LoadHdrImage(const std::string& filename)
{
	return 0;
}

GLuint
TextureSystem::LoadDataTexture(const std::string& filename)
{
	return 0;
}

GLuint
TextureSystem::CreateTexture() const
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
