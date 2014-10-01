#ifndef __BiquadFilter_h__
#define __BiquadFilter_h__

/** Direct Form 1 Cascaded Biquad Filter 
 * Implemented using CMSIS DSP Library
 * Each cascaded stage implements a second order filter
 */
class BiquadFilter {
private:
  arm_biquad_casd_df1_inst_f32 df1;
  float* coefficients;
  /*
   * The <code>pState</code> is a pointer to state array.   
   * Each Biquad stage has 4 state variables <code>x[n-1], x[n-2], y[n-1],</code> and <code>y[n-2]</code>.   
   * The state variables are arranged in the <code>pState</code> array as:   
   * <pre>   
   *     {x[n-1], x[n-2], y[n-1], y[n-2]}   
   * </pre>   
   * The 4 state variables for stage 1 are first, then the 4 state variables for stage 2, and so on.   
   * The state array has a total length of <code>4*numStages</code> values.   
   * The state variables are updated after each block of data is processed; the coefficients are untouched.   
   */
  float* state;
  int stages;
protected:
  void init(int s){
    stages = s;
    coefficients = NULL;
    state = (float*)malloc(stages*4*sizeof(float));
  }
public:
  BiquadFilter() {
    init(1);
  }
  BiquadFilter(int stages) {
    init(stages);
  }
  ~BiquadFilter(){
    free(state);
  }
  /*
   * The coefficients are stored in the array <code>pCoeffs</code> in the following order:
   * <pre>
   *     {b10, b11, b12, a11, a12, b20, b21, b22, a21, a22, ...}
   * </pre>
   * where <code>b1x</code> and <code>a1x</code> are the coefficients for the first stage,
   * <code>b2x</code> and <code>a2x</code> are the coefficients for the second stage,
   * and so on.  The <code>coeffs</code> array must contain a total of <code>5*stages</code> values.   
   */
  void setCoefficents(float* coeffs){
    coefficients = coeffs;
    // note: this also clears the state buffer
    arm_biquad_cascade_df1_init_f32(&df1, stages, coefficients, state);
  }
  /* perform in-place processing */
  void process(float* buf, int size){
    arm_biquad_cascade_df1_f32(&df1, buf, buf, size);
  }
  /* process filter, leaving input intact */
  void process(float* input, float* output, int size){
    arm_biquad_cascade_df1_f32(&df1, input, output, size);
  }
};

/**
   Implements 4x oversampling
*/
class Oversampler {
private:
  BiquadFilter upfilter;
  BiquadFilter downfilter;
  float* oversampled;
public:
  Oversampler(int blocksize) : upfilter(2), downfilter(2) {
    /*   [b,a] = ellip(2, 2, 70, 19200/(48000*4/2)) */
//     static float coeffs[] = { 5.14483600142e-02, 1.01958385114e-01, 5.14483600142e-02, -1.35468949025e+00, 6.12586787851e-01 };
    /* Convert series second-order sections to direct form
     * output_precision(6)
     * [b,a] = ellip(4, 2, 70, 19200/(48000*4/2))
     * [sos,g] = tf2sos(b,a)
     * b0          b1          b2          a0         a1           a2
     * 1.0000000   1.4157500   1.0000000   1.0000000  -1.5499654   0.8890431  first biquad
     * 1.0000000   0.0467135   1.0000000   1.0000000  -1.6359692   0.7189533  second biquad
     */
    #define GAIN 0.0176674622723146
    static float coeffs[10] = { 
        /* If you compute the filter coefficients for one of the filters as e.g. [B A]=ellip(2,2,80,19200/(48000*4/2)) ,
        in the coeffs array you need to put the values {B(0),B(1),B(2),-A(1),-A(2)}.
        As the amplitude is multiplied by 1/4 when upsampling, you will want to compensate for this loss somewhere.
        Either in the filter coefficients (will require different coefficients for upsampling and downsampling), or in the upsample routine (see below)
        Coefficients below were computed in GNU Octave using
        [b,a] = ellip(4, 3, 40, 19200/(48000*4/2)); sos = tf2sos(b,a) */
        0.00319706223776298,   0.00452624091396112,   0.00319706223776297, 1.54996539093296581, -0.88904312844649880,
        1.00000000000000000 ,  0.04671345292281195,   1.00000000000000222, 1.63596919736817048, -0.71895330675421443

          // 1.76674622723146e-02,  -9.17279233954755e-04,   1.76674622723145e-02,  1.58391500480444e+00   -9.32046186768266e-01
          // 1.00000000000000e+00,  -1.29724486401753e+00,   1.00000000000000e+00,  1.65074162038260e+00   -7.48880762739908e-01
    };
    upfilter.setCoefficents(coeffs);
    // two filters: same coefficients, different state variables
    downfilter.setCoefficents(coeffs);
    oversampled = (float*)malloc(blocksize*4*sizeof(float));
  }
  ~Oversampler(){
    free(oversampled);
  }
  float* upsample(float* buf, int sz){
    float* p = oversampled;
    for(int i=0; i<sz; ++i){
      *p++ = buf[i]*4;  /*this *4 compensates for the gain loss due to the zero-stuffing. It turns out that this multiply 
                        does not add to the computational cost, so it is easier to do this rather than 
                        using different filter coefficients for up/down sampling*/
      *p++ = 0;
      *p++ = 0;
      *p++ = 0;
    }
    upfilter.process(oversampled, sz<<2);
    return oversampled;
  }
  float* downsample(float* buf, int sz){
    downfilter.process(oversampled, sz<<2);
    float* p = oversampled;
    for(int i=0; i<sz; ++i){
      buf[i] = *p;
      p += 4;
    }
    return buf;
  }
};

#endif // __BiquadFilter_h__