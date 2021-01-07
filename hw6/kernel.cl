// sampler_t: A type used to control how elements of a 2D or 3D image object are read by read_image{f|i|ui}.
const sampler_t flt_sampler = CLK_NORMALIZED_COORDS_FALSE |
                              CLK_ADDRESS_CLAMP |
                              CLK_FILTER_NEAREST;

__kernel void convolution(__global float *in_img, read_only image2d_t in_flt,
                          __global float *out_img, int W, int H)
{
    // Thread gets its index within index space
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    if (x >= W || y >= H)
        return;

    const int flt_w = get_image_width(in_flt);
    float sum = 0;
    const int shift = flt_w / 2;

    for (int i = 0; i < flt_w; i++)
    {
        for (int j = 0; j < flt_w; j++)
        {
            int nx = x - shift + j;
            int ny = y - shift + i;
            if ((nx < 0) || (nx >= W) || (ny < 0) || (ny >= H))
            {
                continue;
            }
            sum += (read_imagef(in_flt, flt_sampler, (int2)(j, i)).x * in_img[ny * W + nx]);
        }
    }

    out_img[y * W + x] = sum;
}