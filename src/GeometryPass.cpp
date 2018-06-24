#include "GeometryPass.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "MaterialSystem.h"

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
GeometryPass::Draw(const std::vector<Model>& opaqueGeometry, FpsCamera& camera)
{
	// TODO: Do we need to qsort a separate array or can we allow the array to be mixed up?

	// Sort geometry so that we can optimize the number of shader program switches, i.e. calling glUseProgram
	qsort((void *)(opaqueGeometry.data()), opaqueGeometry.size(), sizeof(Model), model_compare_function);

	//
	// TODO: Make sure G-Buffer is set up and ready?
	//

	GLuint lastProgram = UINT_MAX;
	for (const Model& model : opaqueGeometry)
	{
		Material *material = &MaterialSystem::Get(model.materialID);
		if (*material->program != lastProgram)
		{
			lastProgram = *material->program;

			// Camera uniforms
			glUniformMatrix4fv(PredefinedUniformLocation(u_view_from_world), 1, GL_FALSE, glm::value_ptr(camera.GetViewMatrix()));
			glUniformMatrix4fv(PredefinedUniformLocation(u_projection_from_view), 1, GL_FALSE, glm::value_ptr(camera.GetProjectionMatrix()));
		}

		material->BindUniforms();
		model.Draw();
	}

	//
	// TODO:
	//
	//  1. sort geometry based on *material.program, i.e. unique shaders
	//  2. for every new *type of* material, i.e. shader, bind shader specifics such as camera properties
	//  3. for every new material, bind material uniforms, i.e. call BindUniforms()
	//  4. render into G-Buffer, i.e. model.Draw()
	//
}
