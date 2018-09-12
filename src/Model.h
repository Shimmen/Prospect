#pragma once

#include <memory>

#include <glad/glad.h>

#include "Maths.h"
#include "Material.h"

struct Model
{
	GLuint vao = 0;
	GLsizei indexCount;
	GLenum  indexType;

	int transformID = 0;
	BoundingSphere bounds = { {0, 0, 0}, 9999.0f };
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

inline
int TriangleCount(const Model& model)
{
	return model.indexCount / 3;
}