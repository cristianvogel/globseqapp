#include "GlobSeqPlugIn.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "process.hpp"
#include <iostream>
#include <string>
#include <sstream>

GlobSeqPlugIn::GlobSeqPlugIn(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{

    std::cout << std::endl << "Tiny-Process looks for BeSlime" << std::endl;
    std::thread slimeThread( [this] () {
        
    TinyProcessLib::Process zeroConfProcess  ("dns-sd -B _ssh._tcp.", "", [this] (const char *bytes, size_t n)
            {
              std::cout << "Output from stdout: " << std::string(bytes, n);
              beSlimeName = bytes;
      
              //extract beslime name
              std::stringstream ss(beSlimeName);
              std::string line;

              while(std::getline(ss,line,'\n')){
                if(line.find("beslime") != std::string::npos) {
                  const auto beslimeId = line.substr(line.find("beslime") + 8,3);
                  std::cout << "ID: " << beslimeId << std::endl;
                  const auto hardwareName = "beslime-" + beslimeId;
                  beSlimeName = hardwareName;
                }
              }
          });
    });
    slimeThread.detach();
  
   
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = pGraphics->GetBounds();
    pGraphics->AttachControl(new ITextControl(b.GetMidVPadded(50), beSlimeName.c_str(), IText(50)));
    pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(100).GetVShifted(-100), kGain));
  };
  
    
#endif
    
  
}

GlobSeqPlugIn::~GlobSeqPlugIn(){
  // kill the external Process thread in the destructor here when I learn how
}


#if IPLUG_DSP
void GlobSeqPlugIn::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
}
#endif



