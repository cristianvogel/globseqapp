#include "GlobSeqPlugIn.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "process.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <mutex>

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
      pGraphics->AttachPanelBackground(COLOR_NEL_TUNGSTEN);
      pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
      pGraphics->LoadFont("Menlo", MENLO_FN);
      const IVStyle button {
        true, // Show label
        true, // Show value
        {
          DEFAULT_BGCOLOR, // Background
          COLOR_WHITE, // Foreground
          DEFAULT_PRCOLOR, // Pressed
          COLOR_BLACK, // Frame
          DEFAULT_HLCOLOR, // Highlight
          DEFAULT_SHCOLOR, // Shadow
          COLOR_BLACK, // Extra 1
          DEFAULT_X2COLOR, // Extra 2
          DEFAULT_X3COLOR  // Extra 3
        }, // Colors
        IText(8.f, EAlign::Center) // Label text
      };

      
  const IRECT b = pGraphics->GetBounds().GetScaledAboutCentre(0.95f);
      consoleFont = IText ( 12.f, "Menlo");
      
      //▼ small logging console output

      pGraphics->AttachControl
      (new ITextControl(b.GetFromBLHC(PLUG_WIDTH,18).SubRectHorizontal(3, 1), consoleText.c_str(), consoleFont, true), kCtrlNetStatus);
      
      //▼ the right way to make a dial with a paramIdx and ActionFunction
      // in this case an OSC message sending float to one of the oscSender
      //
      pGraphics->AttachControl
                (new NELDoubleDial(
                                   b.GetCentredInside(100).GetVShifted(-100),
                                   {kBPM, kNoParameter}), kCtrlTagBPM
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
                               , kReScan)
                                -> SetActionFunction(
                                                      [this] (IControl* pCaller)
                                                      {
                                                        SplashClickActionFunc(pCaller);

                                                          pCaller->GetUI()->
                                                          GetControlWithTag(kCtrlTagBPM)->
                                                          SetActionFunction ( [this] (IControl* pDialControl)
                                                                             {
                                                                              if (oscSender!=nullptr){
                                                                                                    pDialControl->
                                                                                                             SetParamIdx(kCtrlTagBPM);
                                                                                                             OscMessageWrite msg;
                                                                                                             msg.PushWord("/msg");
                                                                                                             msg.PushFloatArg(pDialControl->GetValue());
                                                                                                             oscSender->SendOSCMessage(msg);
                                                                                                      }
                                                                              }
                                                                            );
                                                          //update console net status
                                                          ITextControl* cnsl = dynamic_cast<ITextControl*>(pCaller->GetUI()->GetControlWithTag(kCtrlNetStatus));
                                                          cnsl->SetStr(consoleText.c_str());
                                                          cnsl->SetDirty();
                                                      }
                                                     );
  
     // this is how to stash the control in a pointer variable
    //  IVButtonControl* rescanButton = dynamic_cast<IVButtonControl*>(pGraphics->GetControlWithTag(kReScan));
      
    };
  
}//end layout function
    
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
GlobSeqPlugIn::~GlobSeqPlugIn() {
  // some kind of naive attempt to kill launchNetworkingThreads()
}


#pragma mark ZeroConfNetworking -
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
                                                       mtx.lock();
                                                        beSlimeName = hardwareName;
                                                       mtx.unlock();
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
                                                                    mtx.lock();
                                                                      oscSender.reset();
                                                                      oscSender =std::make_unique<OSCSender>(); //localhost
                                                                      consoleText = cnsl[kMsgScanning];
                                                                      ipLine.erase();
                                                                      beSlimeConnected = false;
                                                                    mtx.unlock();
                                                                    break;
                                                                 }
                                                                 
                                                                 if (!ipLine.empty()) {
                                                                   if(ipLine.find("beslime") != std::string::npos)
                                                                    {
                                                                       auto ip = (ipLine.substr(ipLine.find(" 169.")+1,16));
                                                                       std::cout << "☑︎ dns-sd extracted IP: " << ip << std::endl;
                                                                       mtx.lock();
                                                                         beSlimeIP = gsh->chomp(ip);
                                                                         cnsl[kMsgConnected] = cnsl[kMsgConnected] + beSlimeName;
                                                                         consoleText = cnsl[kMsgConnected];
                                                                         oscSender = std::make_unique<OSCSender>(beSlimeIP.c_str(), 8000);
                                                                         beSlimeConnected = true;
                                                                       mtx.unlock();
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



