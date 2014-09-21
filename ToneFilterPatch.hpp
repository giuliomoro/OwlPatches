////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 
 
 LICENSE:
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 */

/* created by the OWL team 2014 */


////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __ToneFilterPatch_hpp__
#define __ToneFilterPatch_hpp__
#include "ToneFilter.hpp"
#include "StompBox.h"

class ToneFilterPatch : public Patch {
private:
  ToneFilter tf;
public:
  ToneFilterPatch(){
    registerParameter(PARAMETER_A, "Tone");
    registerParameter(PARAMETER_B, "Gain");
    registerParameter(PARAMETER_C, "Method");
    registerParameter(PARAMETER_E, "Tone");
    float fs=getSampleRate();
    tf=ToneFilter(fs);
  }
  void processAudio(AudioBuffer &buffer){
    static float tone_=0;
    static float gain_=0;
    static float* buf[2];
    float expr = 1 - getParameterValue(PARAMETER_E);
    float tone = getParameterValue(PARAMETER_A)*expr;
    float gain = getParameterValue(PARAMETER_B);
    bool tone1= getParameterValue(PARAMETER_C)<0.5;
    int size = buffer.getSize();
    for (int ch = 0; ch<2; ch++) {//get the pointers to the buffers
      buf[ch] = buffer.getSamples(ch);
    }
    tf.setTone(tone); // placing this here instead of doing it for every sample roughly saves 400 operations per sample, with no audible clicks in the output.
    for (int i = 0; i < size; i++) {                  //process each sample
      gain_=gain*0.001 + gain_*0.999; //parameter smoothing
      for (int ch = 0; ch<2; ch++) {     //for each channel
        buf[ch][i] = gain_*tf.processSample(buf[ch][i], ch);
      }
    }
  }
};

#endif // __ToneFilterPatch_hpp__
