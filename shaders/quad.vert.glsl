#version 460

//
// NOTE: This requires no attributes! Just call
//
//    glDrawArrays(GL_TRIANGLES, 0, 3)
//
// with a valid but "empty" VAO bound
//

out vec2 v_uv;

void main()
{
    // Defines a full screen triangle with CW vertex ordering
    v_uv = vec2(gl_VertexID & 2, (gl_VertexID << 1) & 2);
    gl_Position = vec4(v_uv * 2.0 - 1.0, 0.0, 1.0);
}
