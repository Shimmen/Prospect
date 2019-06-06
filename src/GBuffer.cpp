#include "GBuffer.h"

#include <imgui.h>

#include "Logging.h"
#include "GuiSystem.h"
#include "ShaderSystem.h"
#include "TextureSystem.h"

#include "shader_locations.h"

void
GBuffer::RecreateGpuResources(int width, int height)
{
	this->width = width;
	this->height = height;

	// Docs: "glDeleteTextures silently ignores 0's and names that do not correspond to existing textures."
	glDeleteTextures(1, &albedoTexture);
	glDeleteTextures(1, &materialTexture);
	glDeleteTextures(1, &normVelTexture);
	glDeleteTextures(1, &depthTexture);

	albedoTexture = TextureSystem::CreateTexture(width, height, GL_RGBA8, GL_NEAREST, GL_NEAREST, false);
	materialTexture = TextureSystem::CreateTexture(width, height, GL_RGBA8, GL_NEAREST, GL_NEAREST, false);
	normVelTexture = TextureSystem::CreateTexture(width, height, GL_RGBA16F, GL_NEAREST, GL_NEAREST, false);
	depthTexture = TextureSystem::CreateTexture(width, height, GL_DEPTH_COMPONENT32F, GL_NEAREST, GL_NEAREST, false);

	// Setup the swizzle for the depth textures so all color channels are depth
	GLenum depthSwizzle[] = { GL_RED, GL_RED, GL_RED, GL_ALPHA };
	glTextureParameteriv(depthTexture, GL_TEXTURE_SWIZZLE_RGBA, (GLint *)depthSwizzle);

	if (!framebuffer)
	{
		glCreateFramebuffers(1, &framebuffer);

		GLenum drawBuffers[] = {
			PredefinedOutputLocation(o_g_buffer_albedo),
			PredefinedOutputLocation(o_g_buffer_material),
			PredefinedOutputLocation(o_g_buffer_norm_vel)
		};
		int numDrawBuffers = sizeof(drawBuffers) / sizeof(GLenum);
		glNamedFramebufferDrawBuffers(framebuffer, numDrawBuffers, drawBuffers);
	}

	GLenum albedoAttachment = PredefinedOutputLocation(o_g_buffer_albedo);
	glNamedFramebufferTexture(framebuffer, albedoAttachment, albedoTexture, 0);

	int materialAttachment = PredefinedOutputLocation(o_g_buffer_material);
	glNamedFramebufferTexture(framebuffer, materialAttachment, materialTexture, 0);

	int normVelAttachment = PredefinedOutputLocation(o_g_buffer_norm_vel);
	glNamedFramebufferTexture(framebuffer, normVelAttachment, normVelTexture, 0);

	glNamedFramebufferTexture(framebuffer, GL_DEPTH_ATTACHMENT, depthTexture, 0);

	GLenum status = glCheckNamedFramebufferStatus(framebuffer, GL_DRAW_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		LogError("The G-buffer framebuffer is not complete!");
	}

	// Setup the debug textures

	debugNormalTexture = TextureSystem::CreateTexture(width, height, GL_RGBA8, GL_LINEAR, GL_LINEAR, false);
	debugVelocityTexture = TextureSystem::CreateTexture(width, height, GL_RGBA8, GL_LINEAR, GL_LINEAR, false);

}

void
GBuffer::RenderToDebugTextures() const
{
	glBindImageTexture(0, normVelTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);

	// Debug normal texture
	{
		GLuint filter = *ShaderSystem::AddComputeProgram("etc/visualize_normals.comp.glsl");
		glBindImageTexture(1, debugNormalTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

		glUseProgram(filter);
		glDispatchCompute((width + 32 - 1) / 32, (height + 32 - 1) / 32, 1);
	}

	// Debug velocity texture
	{
		GLuint filter = *ShaderSystem::AddComputeProgram("etc/visualize_velocity.comp.glsl");
		glBindImageTexture(1, debugVelocityTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

		glUseProgram(filter);
		glDispatchCompute((width + 32 - 1) / 32, (height + 32 - 1) / 32, 1);
	}
}

void
GBuffer::RenderGui(const std::string& message) const
{
	if (ImGui::CollapsingHeader(("G-Buffer - " + message).c_str()))
	{
		RenderToDebugTextures();

		ImGui::Text("Albedo:");
		GuiSystem::Texture(albedoTexture);
		ImGui::Text("Material:");
		GuiSystem::Texture(materialTexture);
		ImGui::Text("Normals (debug view):");
		GuiSystem::Texture(debugNormalTexture);
		ImGui::Text("Velocity (debug view):");
		GuiSystem::Texture(debugVelocityTexture);
		ImGui::Text("Depth:");
		GuiSystem::Texture(depthTexture);
	}
}
