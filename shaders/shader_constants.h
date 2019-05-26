#ifndef SHADER_CONSTANTS_H
#define SHADER_CONSTANTS_H

#define SHADOW_MAP_SEGMENT_MAX_COUNT (16)

// How many samples (points in unit sphere) to be defined in the SphereSampleBuffer UBO
#define SPHERE_SAMPLES_COUNT (4096)

// How many layers in the filtered specular IBL roughness map do we want?
#define IBL_RADIANCE_MIPMAP_LAYERS (7)

// The size of the lowest level of the specular IBL radiance map
#define IBL_RADIANCE_BASE_SIZE (1024)

// What will the optimal local size be for the radiance filtering? (given that we want to maximize it)
#define IBL_RADIANCE_OPT_LOCAL_SIZE (IBL_RADIANCE_BASE_SIZE / int(pow(2, IBL_RADIANCE_MIPMAP_LAYERS - 1)))

// Tone map operators:
#define TONEMAP_OP_ACES        (0)
#define TONEMAP_OP_REINHARD    (1)
#define TONEMAP_OP_UNCHARTED_2 (2)
#define TONEMAP_OP_CLAMP       (3)

#endif // SHADER_CONSTANTS_H
