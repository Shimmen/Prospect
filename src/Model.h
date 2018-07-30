#pragma once

#include <memory>

#include <glad/glad.h>

#include "Material.h"

struct Model
{
	GLuint vao;

	GLsizei indexCount;
	GLenum  indexType;

	int transformID = 0;
	Material *material = nullptr;

	void Draw() const
	{
		if (vao)
		{
			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES, indexCount, indexType, nullptr);
		}
	}
};
