#pragma once

#include <glad/glad.h>

// Well, actually a triangle, but this name is more descriptive...
class FullscreenQuad
{
public:

	//
	// Draw a full-screen quad/triangle with an empty vertex array object. The vertices should be specified in the shader as follows:
	//
	//   v_uv = vec2(gl_VertexID & 2, (gl_VertexID << 1) & 2);
	//   gl_Position = vec4(v_uv * 2.0 - 1.0, 0.0, 1.0);
	//
	static void Draw()
	{
		static GLuint emptyVertexArray = 0;
		if (!emptyVertexArray)
		{
			glCreateVertexArrays(1, &emptyVertexArray);
		}

		GLint lastBoundVertexArray;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &lastBoundVertexArray);

		glBindVertexArray(emptyVertexArray);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindVertexArray(lastBoundVertexArray);
	}
	
};
