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
   Implements BF_OS_Lx oversampling
*/
#define BF_OS_L 4
#define LOG2_BF_OS_L 2 //please change this value according to BF_OS_L
class Oversampler {
private:
  BiquadFilter upfilter;
  BiquadFilter downfilter;
  float* oversampled;
public:
  Oversampler(int blocksize) : upfilter(2), downfilter(2) {
    /*  
     * Convert series second-order sections to direct form
     * [b,a] = ellip(4, 2, 70, 19200/(48000*4/2))
     * sos = tf2sos(b,a)
     * b0          b1          b2          a0         a1           a2
     * 1.0000000   1.4157500   1.0000000   1.0000000  -1.5499654   0.8890431  first biquad
     * 1.0000000   0.0467135   1.0000000   1.0000000  -1.6359692   0.7189533  second biquad
     *
     * The signs of the a1 and a2 coefficient need to be changed and the a0 coefficient is omitted (must be a0==1) so that the coefficients array contains 
     * {B(0),B(1),B(2),-A(1),-A(2),...}.
     * As the amplitude is multiplied by 1/BF_OS_L when upsampling, you will want to compensate for this loss somewhere.*/
    static float coeffs[10] = { 
        /*Either in the filter coefficients (will require different coefficients for upsampling and downsampling), or in the upsample routine (see below)
        Coefficients below were computed in GNU Octave using
        [b,a] = ellip(4, 2, 70, 19200/(48000*4/2)); [sos] = tf2sos(b,a)*/
        0.00319706223776298,   0.00452624091396112,   0.00319706223776297, 1.54996539093296581, -0.88904312844649880,
        1.00000000000000000 ,  0.04671345292281195,   1.00000000000000222, 1.63596919736817048, -0.71895330675421443
    };
    upfilter.setCoefficents(coeffs);
    // two filters: same coefficients, different state variables
    downfilter.setCoefficents(coeffs);
    oversampled = (float*)malloc(blocksize*BF_OS_L*sizeof(float));
  }
  ~Oversampler(){
    free(oversampled);
  }
  float* upsample(float* buf, int sz){
    float* p = oversampled;
    for(int i=0; i<sz; ++i){
      *p++ = buf[i]*BF_OS_L;  /*this *BF_OS_L compensates for the gain loss due to the zero-stuffing. It turns out that this multiply 
                        does not add to the computational cost, so it is easier to do this rather than 
                        using different filter coefficients for up/down sampling*/
      for(int i=BF_OS_L-1;i>0;i--){ //set the remaining samples of the oversampled buffer to 0 (zero-stuffing). The for variable i is decreased, hoping this will lead to better compiler optimization
        *p++ = 0;
      }
    }
    upfilter.process(oversampled, sz<<LOG2_BF_OS_L);
    return oversampled;
  }
  float* downsample(float* buf, int sz){
    downfilter.process(oversampled, sz<<LOG2_BF_OS_L);
    float* p = oversampled;
    for(int i=0; i<sz; ++i){
      buf[i] = *p;
      p += BF_OS_L;
    }
    return buf;
  }
};

#endif // __BiquadFilter_h__