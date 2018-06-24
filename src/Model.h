#pragma once

#include <glad/glad.h>

struct Model
{
	GLuint vao;

	GLsizei indexCount;
	GLenum  indexType;

	int transformID = 0;
	int materialID = 0;

	void Draw() const
	{
		if (vao)
		{
			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES, indexCount, indexType, nullptr);
		}
	}
};
