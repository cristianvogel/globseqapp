#include "GlobSeqPlugIn.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "process.hpp"
#include <iostream>
#include <string>
#include <sstream>

GlobSeqPlugIn::GlobSeqPlugIn(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))

{ // start layout function
  launchNetworkingThreads();
  GetParam(kCtrlTagBPM)->InitDouble("BPM", 0., 0., 1000.0, 0.5, "bpm");
 
#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics)
    {
      pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
      pGraphics->AttachPanelBackground(COLOR_GRAY);
      pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
      pGraphics->LoadFont("Menlo", MENLO_FN);
      
  const IRECT b = pGraphics->GetBounds().GetScaledAboutCentre(0.95f);
      consoleFont = IText ( 12.f, "Menlo");
      
      //▼ small logging console output

      pGraphics->AttachControl
      (new ITextControl(b.GetFromBLHC(PLUG_WIDTH,18).SubRectHorizontal(3, 1), consoleText.c_str(), consoleFont, true), kCtrlNetStatus);
      
      //▼ the right way to make a dial with a paramIdx and ActionFunction
      // in this case an OSC message sending float to one of the oscSender
      //
      pGraphics->AttachControl
                (new IVKnobControl(
                                   b.GetCentredInside(100).GetVShifted(-100),
                                   kBPM), kCtrlTagBPM
                 );
      IVKnobControl* bpmDial = dynamic_cast<IVKnobControl*> (pGraphics->GetControlWithTag(kCtrlTagBPM));
      bpmDial -> SetActionFunction (
                                     [this] (IControl* pCaller) {
        
                                                              if (oscSender!=nullptr){
                                                              pCaller->
                                                                       SetParamIdx(kCtrlTagBPM);
                                                                       OscMessageWrite msg;
                                                                       msg.PushWord("/bpm");
                                                                       msg.PushFloatArg(pCaller->GetValue());
                                                                       oscSender->SendOSCMessage(msg);
                                                                }
                                                              }
                                    );
        //bounds, IActionFunction aF = SplashClickActionFunc, const char* label = "", const IVStyle& style = DEFAULT_STYLE, bool labelInButton = true, bool valueInButton = true, EVShape shape = EVShape::Rectangle
      pGraphics->AttachControl( new IVButtonControl(
                                                    b.GetFromBLHC(PLUG_WIDTH, 18.f * 2.f).SubRectHorizontal(12, 0),
                                                    SplashClickActionFunc,
                                                    "rescan",
                                                    DEFAULT_STYLE.WithEmboss(false).WithDrawShadows(false).WithLabelText(consoleFont.WithSize(8.0f).WithAlign(EAlign::Center)),
                                                    true,
                                                    true,
                                                    EVShape::Rectangle
                                                    )
                               , kReScan);
  
      IVButtonControl* rescanButton = dynamic_cast<IVButtonControl*>(pGraphics->GetControlWithTag(kReScan));
      rescanButton -> SetActionFunction(
                                        [this] (IControl* pCaller)
                                        {
                                          SplashClickActionFunc(pCaller);
                                          ITextControl* cnsl = dynamic_cast<ITextControl*>(pCaller->GetUI()->GetControlWithTag(kCtrlNetStatus));
                                          cnsl->SetStr(consoleText.c_str());
                                          cnsl->SetDirty();
                                        }
                                        );
    };
  
}//end layout function
    
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
GlobSeqPlugIn::~GlobSeqPlugIn() {
  stop_scan_thread = true; // naive attempt to kill launchNetworkingThreads()
}

/*
 Launches two threaded system tasks firstly to browse for zeroconfig name of the Kyma hardware
 and secondly to extract the IP address.
 OSCSender is then instantiated with the IP address when known, inside the IP extraction thread.
 Implemented a naive thread kill method scraped from https://stackoverflow.com/questions/12207684/how-do-i-terminate-a-thread-in-c11
 */
void GlobSeqPlugIn::launchNetworkingThreads(){

  std::thread slimeThread( [this] () {
 
       consoleText = cnsl[kMsgScanning];
       TinyProcessLib::Process zeroConfProcess  ("dns-sd -B _ssh._tcp.", "", [this ] (const char *bytes, size_t n)
                                                 {
                                                   std::cout << "\nOutput from zero conf stdout:\n" << std::string(bytes, n);
                                                   beSlimeName = bytes;
                                           
                                                   //extract beslime name
                                                   std::stringstream ss(beSlimeName);
                                                   std::string line;

                                                   while(std::getline(ss,line,'\n'))
                                                   { //start process to extract beslime name
                                                     
                                                     if(line.find("beslime") != std::string::npos)
                                                     {
                                                       const auto beslimeId = line.substr(line.find("beslime") + 8,3);
                                                       const auto hardwareName = "beslime-" + beslimeId;
                                                       std::cout << "☑︎ dns-sd extracted name: " << hardwareName << std::endl;
                                                       beSlimeName = hardwareName;
                                                       if (!line.empty())
                                                       {
                                                           std::cout << std::endl << "Launching IP Scan thread..." << std::endl;
                                                           std::thread slimeIPThread( [this] ()
                                                         { //start process to extract beslime IP
                                                             
                                                             TinyProcessLib::Process ipGrab (
                                                                                             "dns-sd -G v4 " +
                                                                                             beSlimeName + ".local.",
                                                                                             "",
                                                                                             [this] ( const char *bytes, size_t n )
                                                             {
                                                               std::cout << "\nOutput from IP search stdout:\n" << std::string(bytes, n);
                                                               beSlimeIP = bytes;
                                                               std::stringstream ssip(beSlimeIP);
                                                               std::string ipLine;
                                                              
                                                               while(std::getline(ssip,ipLine,'\n'))
                                                               {
                                                                 if(ipLine.find("Rmv") != std::string::npos)
                                                                 {
                                                                    oscSender.reset();
                                                                    oscSender =std::make_unique<OSCSender>(); //localhost
                                                                    consoleText = cnsl[kMsgScanning];
                                                                    ipLine.erase();
                                                                    break;
                                                                 }
                                                                 
                                                                 if (!ipLine.empty()) {
                                                                   if(ipLine.find("beslime") != std::string::npos)
                                                                    {
                                                                     const auto ip = (ipLine.substr(ipLine.find(" 169.")+1,16));
                                                                     std::cout << "☑︎ dns-sd extracted IP: " << ip << std::endl;
                                                                     beSlimeIP = ip;
                                                                     cnsl[kMsgConnected] = cnsl[kMsgConnected] + beSlimeName;
                                                                     consoleText = cnsl[kMsgConnected];
                                                                     oscSender.reset();
                                                                     oscSender = std::make_unique<OSCSender>(beSlimeIP.c_str(), 8000);
                                                                    }
                                                                 }
                                                               }
                                                          return false;   });
                                                           });
                                                        
                                                         slimeIPThread.detach();
                                                       }
                                                     }
                                                   } //outer while loop close
                                             return false;  });
                                            });
 
   slimeThread.detach();
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



