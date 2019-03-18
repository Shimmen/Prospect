#pragma once

#include <glad/glad.h>

#include <vector>

#include "ShaderDependant.h"
#include "LightBuffer.h"

class BloomPass : ShaderDepandant
{
public:

	void Draw(const LightBuffer& lightBuffer);
	void ProgramLoaded(GLuint program) override;

	const int numDownsamples = 6;

	float blurRadius = 0.001f;

	// accessible alias of downsamplingTexture for getting the results
	GLuint bloomResults;

private:

	void Setup(int width, int height);

	GLuint emptyVertexArray{ 0 };
	GLuint *blitProgram;

	GLuint downsamplingTexture;
	GLuint upsamplingTexture;

	GLuint *downsampleProgram;
	GLint dsTargetTexelSizeLoc;
	GLint dsTargetLodLoc;

	GLuint *upsampleProgram;
	GLint usTexelAspectLoc;
	GLint usBlurRadiusLoc;
	GLint usTargetLodLoc;

	std::vector<GLuint> downsamplingFramebuffers;
	std::vector<GLuint> upsamplingFramebuffers;

};
