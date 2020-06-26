#include "GlobSeqPlugIn.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "process.hpp"
#include <iostream>
#include <string>
#include <sstream>

GlobSeqPlugIn::GlobSeqPlugIn(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
, OSCSender("127.0.0.1", 8000)
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
                  auto beslimeId = line.substr(line.find("beslime") + 8,3);
                  auto hardwareName = "beslime-" + beslimeId;
                  std::cout << "ID: " << beslimeId << std::endl;
                  beSlimeName = hardwareName;
                }
              }
          });
    });
    slimeThread.detach();
  
  GetParam(kBPM)->InitDouble("BPM", 0., 0., 1000.0, 0.5, "bpm");

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = pGraphics->GetBounds();
    //▼ label
    pGraphics->AttachControl
              (new ITextControl(b.GetMidVPadded(50), beSlimeName.c_str(), IText(50)));
    
    //▼ the right way to make a dial with a paramIdx and ActionFunction
    // in this case an OSC message sending float
    pGraphics->AttachControl
              (new IVKnobControl(
                                 b.GetCentredInside(100).GetVShifted(-100),
                                 kBPM)
              )->SetActionFunction([this](IControl* pCaller) {
                                     pCaller->SetParamIdx(kBPM);
                                     OscMessageWrite msg;
                                     msg.PushWord("/bpm");
                                     msg.PushFloatArg(pCaller->GetValue());
                                     SendOSCMessage(msg);
                                  }
                        );
  };
  
  std::cout << "Slimer->" + beSlimeName << std::endl;
    
#endif
    
  
}

GlobSeqPlugIn::~GlobSeqPlugIn(){
  // kill the external Process thread in the destructor here when I learn how
}


#if IPLUG_DSP
void GlobSeqPlugIn::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{

  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * 0.0; // mute
    }
  }
}
#endif



