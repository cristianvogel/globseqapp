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
  
  //thread def 2
    std::thread slimeThread( [this] () {
                                        
      TinyProcessLib::Process zeroConfProcess  ("dns-sd -B _ssh._tcp.", "", [this ] (const char *bytes, size_t n)
                                                {
                                                  std::cout << "\nOutput from zero conf stdout:\n" << std::string(bytes, n);
                                                  beSlimeName = bytes;
                                          
                                                  //extract beslime name
                                                  std::stringstream ss(beSlimeName);
                                                  std::string line;

                                                  while(std::getline(ss,line,'\n')){
                                                    if(line.find("beslime") != std::string::npos) {
                                                      const auto beslimeId = line.substr(line.find("beslime") + 8,3);
                                                      const auto hardwareName = "beslime-" + beslimeId;
                                                      std::cout << "ID: " << beslimeId << std::endl;
                                                      beSlimeName = hardwareName;
                                                      if (beslimeId != "") {
                                                          std::cout << std::endl << "Tiny-Process extracts IP" << std::endl;
                                                        //thread def 1 currently not working
                                                          std::thread slimeIPThread( [this] () {
                                                            TinyProcessLib::Process ipGrab ("dns-sd -G v4 " + beSlimeName + ".local.", "", [this] (const char *bytes, size_t n)
                                                            {
                                                                  std::cout << "\nOutput from stdout IP grabber: \n" << std::string(bytes, n);
                                                                  beSlimeIP = bytes;
                                                                  
                                                              //extract beslime IP
                                                              std::stringstream ssip(beSlimeIP);
                                                              std::string ipLine;
                                                              while(std::getline(ssip,ipLine,'\n')){
                                                                if(ipLine.find("beslime") != std::string::npos) {
                                                                  const auto ip = (ipLine.substr(ipLine.find(" 169.")+1,16));
                                                                  std::cout << "IP: " << ip << std::endl;
                                                                  beSlimeIP = ip;
                                                                  
                                                                  //todo: I'd like to assign the new destination IP here from within the networking thread
                                                                }
                                                              }
                                                            });
                                                          });
                                                        slimeIPThread.detach();
                                                      }
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
}
    

GlobSeqPlugIn::~GlobSeqPlugIn(){
  
  // kill the external TinyProcess threads in the destructor here when I learn how
}
#endif


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



