#ifndef SSAO_DATA_H
#define SSAO_DATA_H

// The number of samples in the SSAO kernel
#define SSAO_KERNEL_SAMPLE_COUNT (16)

struct SSAOData
{
    // xyz - kernel position, w - unused/padding
    vec4  kernel[SSAO_KERNEL_SAMPLE_COUNT];

    float kernel_radius;
    float intensity;
};

#endif // SSAO_DATA_H
