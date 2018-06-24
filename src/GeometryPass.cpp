#include "GeometryPass.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Logging.h"
#include "MaterialSystem.h"
#include "TransformSystem.h"

#include "shader_locations.h"

int
model_compare_function(const void *a, const void *b)
{
	auto modelA = (const Model *)a;
	auto modelB = (const Model *)b;

	auto materialA = &MaterialSystem::Get(modelA->materialID);
	auto materialB = &MaterialSystem::Get(modelB->materialID);

	if (*materialA->program <  *materialB->program) return -1;
	if (*materialA->program == *materialB->program) return  0;
	if (*materialA->program >  *materialB->program) return +1;

	return 0;
}

void
GeometryPass::Draw(const GBuffer& gBuffer, const std::vector<Model>& opaqueGeometry, FpsCamera& camera)
{
	// Prepare framebuffer
	{
		if (!framebuffer)
		{
			glCreateFramebuffers(1, &framebuffer);
		}

		if (gBuffer.albedoTexture != lastBoundAlbedo)
		{
			GLenum albedoAttachment = PredefinedOutputLocation(o_g_buffer_albedo);
			glNamedFramebufferTexture(framebuffer, albedoAttachment, gBuffer.albedoTexture, 0);
			lastBoundAlbedo = gBuffer.albedoTexture;
		}

		if (gBuffer.normalTexture != lastBoundNormal)
		{
			int normalAttachment = PredefinedOutputLocation(o_g_buffer_normal);
			glNamedFramebufferTexture(framebuffer, normalAttachment, gBuffer.normalTexture, 0);
			lastBoundNormal = gBuffer.normalTexture;
		}

		if (gBuffer.depthTexture != lastBoundDepth)
		{
			glNamedFramebufferTexture(framebuffer, GL_DEPTH_ATTACHMENT, gBuffer.depthTexture, 0);
			lastBoundDepth = gBuffer.depthTexture;
		}

		GLenum status = glCheckNamedFramebufferStatus(framebuffer, GL_DRAW_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			LogError("G-buffer is not complete for the geometry pass!");
		}

		//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}

	//
	// TODO: Do we need to qsort a separate array or can we allow the array to be mixed up?
	//

	// Sort geometry so that we can optimize the number of shader program switches, i.e. calling glUseProgram
	qsort((void *)(opaqueGeometry.data()), opaqueGeometry.size(), sizeof(Model), model_compare_function);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	GLuint lastProgram = UINT_MAX;
	for (const Model& model : opaqueGeometry)
	{
		Material *material = &MaterialSystem::Get(model.materialID);
		GLuint program = *material->program;

		if (program != lastProgram)
		{
			glUseProgram(program);
			lastProgram = program;
		}

		Transform& transform = TransformSystem::Get(model.transformID);
		material->BindUniforms(transform);

		model.Draw();
	}
}
