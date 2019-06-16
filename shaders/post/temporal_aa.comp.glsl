#version 460

/*
This shader is a modified version of Eric Arnebäck's original TAA shader, wich can
be found here: https://gist.github.com/Erkaman/f24ef6bd7499be363e6c99d116d8734d.
This version is a compute shader version of the original.

Original license:

The MIT License (MIT)
Copyright (c) 2018 Eric Arnebäck
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <common.glsl>

layout(
    local_size_x = 32,
    local_size_y = 32
) in;

layout(binding = 1, rgba32f) restrict readonly  uniform image2D img_src;
layout(binding = 2, rgba16f) restrict readonly  uniform image2D img_norm_vel;

layout(binding = 0, rgba32f) restrict writeonly uniform image2D   img_dst;
layout(binding = 0)                             uniform sampler2D u_history_texture;


uniform float u_history_blend = 0.05;
uniform bool u_first_frame = true;

void main()
{
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imgSize = ivec2(imageSize(img_src).xy);

    if (pixelCoord.x < imgSize.x && pixelCoord.y < imgSize.y)
    {
        vec3 color;

        if (u_first_frame)
        {
            // first frame, no blending at all
            color = imageLoad(img_src, pixelCoord).rgb;
        }
        else
        {
            vec3 neighbourhood[9];

            // TODO: Implement edge clamp!?
            neighbourhood[0] = imageLoad(img_src, pixelCoord + ivec2(-1, -1)).rgb;
            neighbourhood[1] = imageLoad(img_src, pixelCoord + ivec2( 0, -1)).rgb;
            neighbourhood[2] = imageLoad(img_src, pixelCoord + ivec2(+1, -1)).rgb;
            neighbourhood[3] = imageLoad(img_src, pixelCoord + ivec2(-1,  0)).rgb;
            neighbourhood[4] = imageLoad(img_src, pixelCoord + ivec2( 0,  0)).rgb;
            neighbourhood[5] = imageLoad(img_src, pixelCoord + ivec2(+1,  0)).rgb;
            neighbourhood[6] = imageLoad(img_src, pixelCoord + ivec2(-1, +1)).rgb;
            neighbourhood[7] = imageLoad(img_src, pixelCoord + ivec2( 0, +1)).rgb;
            neighbourhood[8] = imageLoad(img_src, pixelCoord + ivec2(+1, +1)).rgb;

            vec3 nmin = neighbourhood[0];
            vec3 nmax = neighbourhood[0];
            for(int i = 1; i < 9; ++i)
            {
                nmin = min(nmin, neighbourhood[i]);
                nmax = max(nmax, neighbourhood[i]);
            }

            vec2 velocity = imageLoad(img_norm_vel, pixelCoord).zw;
            vec2 pixelUv = (vec2(pixelCoord) + 0.5) / vec2(imgSize);
            vec2 histUv = pixelUv - velocity;

            // sample from history buffer, with neighbourhood clamping.
            vec3 histSample = textureLod(u_history_texture, histUv, 0).rgb;
            histSample = clamp(histSample, nmin, nmax);

            bvec2 a = greaterThan(histUv, vec2(1.0));
            bvec2 b = lessThan(histUv, vec2(0.0));
            // if history sample is outside screen, switch to aliased image as a fallback.
            float blend = any(bvec2(any(a), any(b))) ? 1.0 : u_history_blend;

            vec3 currSample = neighbourhood[4];
            // finally, blend current and clamped history sample.
            color = mix(histSample, currSample, vec3(blend));
        }

        imageStore(img_dst, pixelCoord, vec4(color, 1.0));
    }
}
