#ifndef CAMERA_MODEL_GLSL
#define CAMERA_MODEL_GLSL

//
// The following three functions are from the 'Moving Frostbite to PBR' course notes:
// https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
//

float computeEV100(float aperture, float shutterTime, float ISO)
{
    // EV number is defined as:
    //   2^EV_s = N^2 / t and EV_s = EV_100 + log2(S/100)
    // This gives
    //   EV_s = log2(N^2 / t)
    //   EV_100 + log2(S/100) = log2 (N^2 / t)
    //   EV_100 = log2(N^2 / t) - log2(S/100)
    //   EV_100 = log2(N^2 / t . 100 / S)
    return log2(square(aperture) / shutterTime * 100.0 / ISO);
}

float computeEV100FromAvgLuminance(float avgLuminance)
{
    // We later use the middle gray at 12.7% in order to have
    // a middle gray at 18% with a sqrt (2) room for specular highlights
    // But here we deal with the spot meter measuring the middle gray
    // which is fixed at 12.5 for matching standard camera
    // constructor settings (i.e. calibration constant K = 12.5)
    // Reference : http://en.wikipedia.org/wiki/Film_speed
    return log2(avgLuminance * 100.0 / 12.5);
}

float convertEV100ToExposure(float EV100)
{
    // Compute the maximum luminance possible with H_sbs sensitivity
    // maxLum = 78 / (  S * q   ) * N^2 / t
    //        = 78 / (  S * q   ) * 2^EV_100
    //        = 78 / (100 * 0.65) * 2^EV_100
    //        = 1.2 * 2^EV
    // Reference : http://en.wikipedia.org/wiki/Film_speed
    float maxLuminance = 1.2 * pow(2.0, EV100);
    return 1.0 / maxLuminance;
}

float naturalVignetting(float falloff, float aspectRatio, vec2 uv)
{
    // From: https://github.com/keijiro/KinoVignette

    vec2 coord = (uv - vec2(0.5)) * vec2(aspectRatio, 1.0) * 2.0;
    float rf = sqrt(dot(coord, coord)) * falloff;
    float rf2_1 = rf * rf + 1.0;
    float e = 1.0 / (rf2_1 * rf2_1);

    return e;
}


const float whiteValue = 11.2;
vec3 uncharted2Tonemap(vec3 x)
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;

    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 tonemap(vec3 hdrColor)
{
    float exposureBias = 1.0;
    vec3 curr = uncharted2Tonemap(exposureBias * hdrColor);

    vec3 whiteScale = vec3(1.0) / uncharted2Tonemap(vec3(whiteValue));
    vec3 color = curr * whiteScale;

    return color;
}

vec3 gammaAdust(vec3 color, float gamma)
{
    return pow(color, vec3(1.0 / gamma));
}

#endif // CAMERA_MODEL_GLSL
