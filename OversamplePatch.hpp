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
/*
This patch does not do anything, only upsamples and downsamples the input signal. Used for computational cost measurements.
It can be used as a very expensive buffer if you want to plug your headphones into your passive electric guitar :D 
Parameter_A toggles between oversampled(when >0.5) and non-oversampled(when<0.5) 
*/

#ifndef __OversamplePatch_hpp__
#define __OversamplePatch_hpp__
#include "Oversample.hpp"
#include "StompBox.h"

class OversamplePatch : public Patch {
private:
  Oversample os;
  float* buf[2];
public:
  OversamplePatch(){
    registerParameter(PARAMETER_A, "Toggle oversample");
  }
  void processAudio(AudioBuffer &buffer){
    int size = buffer.getSize();
    for (int ch = 0; ch<1; ch++) {//get the pointers to the buffers
      buf[ch] = buffer.getSamples(ch);
    }
    if(getParameterValue(PARAMETER_A)>0.5) { //only oversample if the knob is >0.5
      for (int i = 0; i < size; i++) { //process each sample
        for (int ch = 0; ch<1; ch++) { //for each channel
          buf[ch][i] = os.processSample(buf[ch][i]);
        }
      }
    }
  }
};

#endif // __OversamplePatch_hpp__
