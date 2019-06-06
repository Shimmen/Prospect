#include "SkyPass.h"

#include "ShaderSystem.h"
#include "FullscreenQuad.h"

#include "shader_locations.h"

void
SkyPass::Draw(const LightBuffer& lightBuffer, const GBuffer& gBuffer, Scene& scene)
{
	if (!skyProgram)
	{
		ShaderSystem::AddProgram("light/skybox", this);

		glCreateFramebuffers(1, &framebuffer);

		// TODO: Why this??! Attachment 1 is not in use, but we seem to need it?
		//GLenum drawBuffers[] = { PredefinedOutputLocation(o_color), PredefinedOutputLocation(o_g_buffer_norm_vel) };
		//glNamedFramebufferDrawBuffers(framebuffer, sizeof(drawBuffers) / sizeof(drawBuffers[0]), drawBuffers);
		GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glNamedFramebufferDrawBuffers(framebuffer, 3, drawBuffers);
	}

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_EQUAL);
	glDepthMask(GL_FALSE);

	// TODO: Maybe don't rebind all textures every frame?
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
	glNamedFramebufferTexture(framebuffer, PredefinedOutputLocation(o_color), lightBuffer.lightTexture, 0);
	glNamedFramebufferTexture(framebuffer, PredefinedOutputLocation(o_g_buffer_norm_vel), gBuffer.normVelTexture, 0);
	glNamedFramebufferTexture(framebuffer, GL_DEPTH_ATTACHMENT, gBuffer.depthTexture, 0);
	glViewport(0, 0, lightBuffer.width, lightBuffer.height);

	glUseProgram(skyProgram);
	glBindTextureUnit(0, scene.skyProbe.radiance);
	FullscreenQuad::Draw();

	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
}

void
SkyPass::ProgramLoaded(GLuint program)
{
	skyProgram = program;
	glProgramUniform1i(skyProgram, PredefinedUniformLocation(u_texture), 0);
}
