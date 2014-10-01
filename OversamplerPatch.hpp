/*
This patch does not do anything, only upsamples and downsamples the input signal. Used for computational cost measurements.
*/

#ifndef __OversamplerPatch_hpp__
#define __OversamplerPatch_hpp__
// #include "Oversample.hpp"
#include "StompBox.h"
#include "BiquadFilter.hpp"

class OversamplerPatch : public Patch {
private:
  Oversampler os;
public:
  OversamplerPatch() : os(getBlockSize()) {
    registerParameter(PARAMETER_A, "Toggle oversample");
  }
  void processAudio(AudioBuffer &buffer){
    if(getParameterValue(PARAMETER_A)<0.5) { //bypass;
      return;
    }
    int size = buffer.getSize();
    float* buf = buffer.getSamples(0);
    float* upsampled = os.upsample(buf, size);
    // for(int i = 0; i < size*4; i++) {
      // upsampled[i] = oversampledFunction(upsampled[i]);
    // }
    os.downsample(buf, size);
  }
  // float oversampledFunction(float x){ 
    // return x * ( 27 + x*x ) / ( 27 + 9*x*x ); 
  // }
};

#endif // __OversamplerPatch_hpp__