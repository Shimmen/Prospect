#include "PointcloudExplorer.h"

#include <ios>
#include <fstream>

#include "FpsCamera.h"
#include "GuiSystem.h"
#include "ShaderSystem.h"

#include "etc/pointcloud_data.h"

using namespace glm;

///////////////////////////////////////////////////////////////////////////////
// Pointcloud related

struct Pointcloud
{
	// xyz positions
	float *positions;

	// rgb color + intensity on alpha
	uint8_t *colors;

	uint32_t pointCount;
};

Pointcloud LoadPointcloud(const char *filename)
{
	std::ifstream ifs{ filename };

	constexpr size_t maxLineSize = 256;
	char line[maxLineSize]{};

	size_t maxPoints = std::count(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>(), '\n');
	printf("Done, %zi points\n", maxPoints);
	ifs.seekg(0, std::ios::beg);

	Pointcloud pc{};
	pc.positions = static_cast<float *>(malloc(3 * sizeof(float) * maxPoints));
	pc.colors = static_cast<uint8_t *>(malloc(4 * sizeof(uint8_t) * maxPoints));

	uint32_t i = 0;
	while (!ifs.eof())
	{
		ifs.getline(line, maxLineSize);

		if (line[0] == '#')
		{
			continue;
		}

		int intensity;
		sscanf_s(line, "%f %f %f %u %hhu %hhu %hhu",
			&pc.positions[3 * i + 0],
			&pc.positions[3 * i + 2],
			&pc.positions[3 * i + 1],
			&intensity,
			&pc.colors[4 * i + 0],
			&pc.colors[4 * i + 1],
			&pc.colors[4 * i + 2]);

		intensity += 2048;         // offset from [-2048, 2048) to [0, 4096)
		intensity /= (4096 / 256); // bias from [0, 4096) to [0, 256)

		pc.colors[4 * i + 3] = intensity;

		i += 1;
	}

	pc.pointCount = i;
	printf("Actual count: %u\n", pc.pointCount);

	return pc;
}

void WriteBinaryPointcloud(const Pointcloud& pointcloud, const char *filename)
{
	std::ofstream binary(filename, std::ios::out | std::ios::binary);

	binary.write((char *)&pointcloud.pointCount, sizeof(pointcloud.pointCount));

	size_t positionsByteSize = 3 * sizeof(float) * pointcloud.pointCount;
	binary.write((char *)pointcloud.positions, positionsByteSize);

	size_t colorsByteSize = 4 * sizeof(uint8_t) * pointcloud.pointCount;
	binary.write((char *)pointcloud.colors, colorsByteSize);
}

Pointcloud ReadBinaryPointcloud(const char *filename)
{
	std::ifstream binary(filename, std::ios::in | std::ios::binary);

	Pointcloud pc{};
	binary.read((char *)&pc.pointCount, sizeof(pc.pointCount));
	pc.positions = static_cast<float *>(malloc(3 * sizeof(float) * pc.pointCount));
	pc.colors = static_cast<uint8_t *>(malloc(4 * sizeof(uint8_t) * pc.pointCount));

	size_t positionsByteSize = 3 * sizeof(float) * pc.pointCount;
	binary.read((char *)pc.positions, positionsByteSize);

	size_t colorsByteSize = 4 * sizeof(float) * pc.pointCount;
	binary.read((char *)pc.colors, colorsByteSize);

	return pc;
}

///////////////////////////////////////////////////////////////////////////////
// Data

namespace
{
	Pointcloud pointcloud;
	FpsCamera camera{};

	GLuint vertexArray;
	GLuint positionsBuffer;
	GLuint colorsBuffer;

	PointcloudData data = {};
	GLuint dataBuffer;

	GLuint *pointcloudProgram = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
// Application lifetime

App::Settings PointcloudExplorer::Setup()
{
	Settings settings{};
	settings.window.size = { 1280, 800 };
	settings.window.vsync = false;
	settings.window.resizeable = true;
	settings.context.msaaSamples = 1;
	return settings;
}

void PointcloudExplorer::Init()
{
	//pointcloud = LoadPointcloud("assets/pointcloud/untermaederbrunnen_station1_xyz_intensity_rgb.txt");
	//WriteBinaryPointcloud(pointcloud, "assets/pointcloud/untermaederbrunnen_station1.bin");
	pointcloud = ReadBinaryPointcloud("assets/pointcloud/untermaederbrunnen_station1.bin");

	//

	glCreateBuffers(1, &positionsBuffer);
	glNamedBufferData(positionsBuffer, 3 * sizeof(float) * pointcloud.pointCount, pointcloud.positions, GL_STATIC_DRAW);

	glCreateBuffers(1, &colorsBuffer);
	glNamedBufferData(colorsBuffer, 3 * sizeof(uint8_t) * pointcloud.pointCount, pointcloud.colors, GL_STATIC_DRAW);

	glCreateVertexArrays(1, &vertexArray);

	// Bind the vertex array to a specific binding index and specify it stride, etc.
	//const GLuint vertexArrayBindingIndex = 0;
	glVertexArrayVertexBuffer(vertexArray, 0, positionsBuffer, 0, 3 * sizeof(float));
	glVertexArrayVertexBuffer(vertexArray, 1, colorsBuffer, 0, 4 * sizeof(uint8_t));

	// Enable the attribute, specify its format, and connect the vertex array (at its
	// binding index) to to this specific attribute for this vertex array
	glEnableVertexArrayAttrib(vertexArray, 0);
	glVertexArrayAttribFormat(vertexArray, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vertexArray, 0, 0);
	glVertexArrayBindingDivisor(vertexArray, 0, 1);

	glEnableVertexArrayAttrib(vertexArray, 1);
	glVertexArrayAttribFormat(vertexArray, 1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0);
	glVertexArrayAttribBinding(vertexArray, 1, 1);
	glVertexArrayBindingDivisor(vertexArray, 1, 1);

	//

	data.base_scale = 0.005f;

	glCreateBuffers(1, &dataBuffer);
	glNamedBufferData(dataBuffer, sizeof(data), &data, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 13, dataBuffer);

	//

	pointcloudProgram = ShaderSystem::AddProgram("etc/pointcloud");

	//

	camera.LookAt({ -10.5f, 6.8f, 1.0f }, { -20.0f, 0.0f, +15.0f });
}

void PointcloudExplorer::Resize(int width, int height)
{
	camera.Resize(width, height);
	glViewport(0, 0, width, height);
}

///////////////////////////////////////////////////////////////////////////////
// Drawing / main loop

void PointcloudExplorer::Draw(const Input& input, float deltaTime, float runningTime)
{
	ImGui::Begin("Pointcloud Explorer");
	ImGui::Text("Frame time: %.1f ms", deltaTime * 1000);

	camera.Update(input, deltaTime);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0, 0, 0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	//glDisable(GL_BLEND);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	if (pointcloudProgram)
	{
		glUseProgram(*pointcloudProgram);

		ImGui::SliderFloat("Base scale", &data.base_scale, 0.0001f, 0.1f);

		data.time = runningTime;
		data.camera_position = { camera.GetPosition(), 0.0f };
		data.projection_from_world = camera.GetProjectionMatrix() * camera.GetViewMatrix();
		glNamedBufferSubData(dataBuffer, 0, sizeof(data), &data);

		glBindVertexArray(vertexArray);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 14, pointcloud.pointCount);
	}

	ImGui::End();
}

///////////////////////////////////////////////////////////////////////////////
