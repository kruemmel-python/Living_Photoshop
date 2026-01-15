#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int halo_init_features();

int halo_img_rgb_u8_to_f32_interleaved(
    const unsigned char* src, long long src_stride,
    float* dst, long long dst_stride,
    int width, int height,
    float scale, float offset,
    float alpha, float beta,
    int use_mt
);

int halo_sobel_f32(
    const float* src, long long src_stride,
    float* dst, long long dst_stride,
    int width, int height,
    int use_mt
);

int halo_gaussian_blur_f32(
    const float* src, long long src_stride,
    float* dst, long long dst_stride,
    int width, int height,
    float sigma,
    int use_mt
);

int halo_unsharp_mask_f32(
    const float* src, long long src_stride,
    float* dst, long long dst_stride,
    int width, int height,
    float sigma,
    float amount,
    float threshold,
    int use_mt
);

int halo_box_blur_f32(
    const float* src, long long src_stride,
    float* dst, long long dst_stride,
    int width, int height,
    int radius,
    int use_mt
);

#ifdef __cplusplus
}
#endif
