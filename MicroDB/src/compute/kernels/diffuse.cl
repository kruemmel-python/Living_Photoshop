__kernel void diffuse_and_evaporate(__global const float *input,
                                    __global float *output,
                                    int width,
                                    int height,
                                    float diffusion,
                                    float evaporation) {
    int x = (int)get_global_id(0);
    int y = (int)get_global_id(1);
    if (x >= width || y >= height) {
        return;
    }
    int idx = y * width + x;
    float center = input[idx];

    if (x == 0 || y == 0 || x == width - 1 || y == height - 1) {
        float value = center * (1.0f - evaporation);
        output[idx] = fmax(value, 0.0f);
        return;
    }

    float sum = center * (1.0f - diffusion);
    sum += input[idx - 1] * (diffusion * 0.25f);
    sum += input[idx + 1] * (diffusion * 0.25f);
    sum += input[idx - width] * (diffusion * 0.25f);
    sum += input[idx + width] * (diffusion * 0.25f);

    float value = sum * (1.0f - evaporation);
    output[idx] = fmax(value, 0.0f);
}

__kernel void diffuse_and_evaporate_3d(__global const float *input,
                                       __global float *output,
                                       int width,
                                       int height,
                                       int depth,
                                       float diffusion,
                                       float evaporation) {
    int x = (int)get_global_id(0);
    int y = (int)get_global_id(1);
    int z = (int)get_global_id(2);
    if (x >= width || y >= height || z >= depth) {
        return;
    }
    int slice = width * height;
    int idx = z * slice + y * width + x;
    float center = input[idx];

    if (x == 0 || y == 0 || z == 0 || x == width - 1 || y == height - 1 || z == depth - 1) {
        float value = center * (1.0f - evaporation);
        output[idx] = fmax(value, 0.0f);
        return;
    }

    float sum = center * (1.0f - diffusion);
    float w = diffusion / 6.0f;
    sum += input[idx - 1] * w;
    sum += input[idx + 1] * w;
    sum += input[idx - width] * w;
    sum += input[idx + width] * w;
    sum += input[idx - slice] * w;
    sum += input[idx + slice] * w;

    float value = sum * (1.0f - evaporation);
    output[idx] = fmax(value, 0.0f);
}
