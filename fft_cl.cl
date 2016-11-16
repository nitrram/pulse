
__kernel void fft(__global uchar4 *out,
                  __global uchar4 *inp,
                  int dens) {

   int x = get_global_id(0);
   int siz = get_global_size(0);

   /* z ~ [R|G|B] */

   //  int r = (y%8 && x%8) ? 0 : 120;

   //dst_buf[oi] = dist_colors(z); //(uchar4)(0,0,r,0);
}
