#include "TemporalAAPass.h"

#include <imgui.h>

#include "PerformOnce.h"
#include "ShaderSystem.h"

void TemporalAAPass::Draw(const LightBuffer& lightBuffer, const GBuffer& gBuffer)
{
	if (ImGui::CollapsingHeader("Temporal AA"))
	{
		ImGui::Checkbox("Enabled", &enabled);
		ImGui::SliderFloat("History blend", &historyBlend.value, 0.0f, 1.0f, "%.3f", 3.0f);
	}

	// NOTE: we also want to count frames that we don't actually render on!
	frameCount += 1;

	if (!enabled)
	{
		// Just fully pass through
		outputTexture = lightBuffer.lightTexture;
		return;
	}

	PerformOnce(
		taaProgram = ShaderSystem::AddComputeProgram("post/temporal_aa.comp.glsl", this);
	)

	glUseProgram(*taaProgram);
	historyBlend.UpdateUniformIfNeeded(*taaProgram);

	glBindImageTexture(1, lightBuffer.lightTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(2, gBuffer.normVelTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);

	// Read/write from different history buffers depending on even/odd frames
	static int evenOdd = 0;
	evenOdd = (evenOdd + 1) % 2;

	GLuint inputTexture;
	if (evenOdd == 0)
	{
		inputTexture = lightBuffer.taaHistoryTextures[0];
		outputTexture = lightBuffer.taaHistoryTextures[1];
	}
	else
	{
		inputTexture = lightBuffer.taaHistoryTextures[1];
		outputTexture = lightBuffer.taaHistoryTextures[0];
	}
	
	glBindTextureUnit(0, inputTexture);
	glBindImageTexture(0, outputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	bool firstFrameForCurrentRun = ShouldSetFirstFrame(lightBuffer.width, lightBuffer.height, frameCount);
	glProgramUniform1i(*taaProgram, firstFrameLocation, firstFrameForCurrentRun);

	int xGroups = int(ceil(lightBuffer.width / 32.0f));
	int yGroups = int(ceil(lightBuffer.height / 32.0f));
	glDispatchCompute(xGroups, yGroups, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void TemporalAAPass::ProgramLoaded(GLuint program)
{
	if (taaProgram && program == *taaProgram)
	{
		firstFrameLocation = glGetUniformLocation(*taaProgram, "u_first_frame");
	}
}

bool TemporalAAPass::ShouldSetFirstFrame(int width, int height, int frameCount) const
{
	static int lastFrameCount = -1;
	static int lastWidth = 0;
	static int lastHeight = 0;

	int diff = frameCount - lastFrameCount;
	lastFrameCount = frameCount;

	if (diff > 1 || width != lastWidth || height != lastHeight)
	{
		lastWidth = width;
		lastHeight = height;
		return true;
	}
	
	return false;
}
