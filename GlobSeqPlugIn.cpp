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

{

  launchNetworkingThreads();
  GetParam(kFluxDial1)->InitDouble("FluxType", 0., 0., 1.f, 0.f, "FluxType");
  GetParam(kFluxDial1)->InitDouble("FluxRange", 0., 0., 1.f, 0.f, "FluxRange");
  GetParam(kFluxDial2)->InitDouble("FluxType", 0., 0., 1.f, 0.f, "FluxType");
  GetParam(kFluxDial2)->InitDouble("FluxRange", 0., 0., 1.f, 0.f, "FluxRange");
  GetParam(kFluxDial3)->InitDouble("FluxType", 0., 0., 1.f, 0.f, "FluxType");
  GetParam(kFluxDial3)->InitDouble("FluxRange", 0., 0., 1.f, 0.f, "FluxRange");
  GetParam(kFluxDial4)->InitDouble("FluxType", 0., 0., 1.f, 0.f, "FluxType");
  GetParam(kFluxDial4)->InitDouble("FluxRange", 0., 0., 1.f, 0.f, "FluxRange");
 
#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics)
    { // start layout lambda function
      pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
      pGraphics->AttachPanelBackground(COLOR_NEL_TUNGSTEN);
      pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
      pGraphics->LoadFont("Menlo", MENLO_FN);
      
      const IVStyle rescanButtonStyle {
        true, // Show label
        false, // Show value
        {
          DEFAULT_SHCOLOR, // Background
          COLOR_TRANSLUCENT, // Foreground
          COLOR_LIGHT_GRAY, // Pressed
          COLOR_TRANSPARENT, // Frame
          DEFAULT_HLCOLOR, // Highlight
          DEFAULT_SHCOLOR, // Shadow
          COLOR_BLACK, // Extra 1
          DEFAULT_X2COLOR, // Extra 2
          DEFAULT_X3COLOR  // Extra 3
        }, // Colors
        IText(
              10.f,
              COLOR_LIGHT_GRAY,
              "Menlo",
              EAlign::Center,
              EVAlign::Middle,
              0.f,
              DEFAULT_TEXTENTRY_BGCOLOR,
              DEFAULT_TEXTENTRY_FGCOLOR
              ) // Label text
      };
      
      // main app GUI IRECT
      const IRECT b = pGraphics->GetBounds().GetScaledAboutCentre(0.95f);
      const IRECT consoleBounds = b.GetFromBLHC(PLUG_WIDTH,18).SubRectHorizontal(3, 1);
                                                           
      /*
          Possibly move from ITextControl to IVLabelControl in order to be able to apply styles for example:
          IVLabelControl::IVLabelControl(const IRECT& bounds, const char* label, const IVStyle& style)
      */
            
                 //▼ small logging console output
                 consoleFont = IText ( 12.f, "Menlo").WithFGColor(COLOR_NEL_TUNGSTEN_FGBlend);
                 pGraphics->AttachControl(new ITextControl(consoleBounds, consoleText.c_str(), consoleFont, true), kCtrlNetStatus);
                 
                 pGraphics->AttachControl(new ILambdaControl(
                                                             b.GetFromBLHC(PLUG_WIDTH,18).SubRectHorizontal(3, 1),
                                                             [this](ILambdaControl* pCaller, IGraphics& g, IRECT& rect)
                                                               {
                                                                 ITextControl* cnsl = dynamic_cast<ITextControl*>(g.GetControlWithTag(kCtrlNetStatus));
                                                                 cnsl->SetStr(consoleText.c_str());
                                                                 cnsl->SetDirty();
                                                               },
                                                             DEFAULT_ANIMATION_DURATION, true /*loop*/, false /*start imediately*/));
      
      //▼ concentric dials with two paramIdx
    
      pGraphics->AttachControl
                (new NELDoubleDial(
                                   b.GetGridCell(0, 0, 2, 2).GetCentredInside(100),
                                   {kFluxDial1Inner, kFluxDial1Outer}), kFluxDial1
                 );
      pGraphics->AttachControl
                (new NELDoubleDial(
                                   b.GetGridCell(1, 0, 2, 2).GetCentredInside(100),
                                   {kFluxDial2Inner, kFluxDial2Outer}), kFluxDial2
                 );
      pGraphics->AttachControl
                (new NELDoubleDial(
                                   b.GetGridCell(0, 1, 2, 2).GetCentredInside(100),
                                   {kFluxDial3Inner, kFluxDial3Outer}), kFluxDial3
                 );
      pGraphics->AttachControl
                (new NELDoubleDial(
                                   b.GetGridCell(1, 1, 2, 2).GetCentredInside(100),
                                   {kFluxDial4Inner, kFluxDial4Outer}), kFluxDial4
                 );

      //▼ Scan button attaches OSC ActionFunction lambda to concentric dials, button is atop kCtrlNetStatus
      pGraphics->AttachControl( new IVButtonControl(
                                                      consoleBounds,
                                                      nullptr,
                                                      "",
                                                      rescanButtonStyle.WithEmboss(false).WithDrawShadows(false),
                                                      true,
                                                      true,
                                                      EVShape::Rectangle
                                                    )
                               , kCtrlReScan)
                                -> SetActionFunction(
                                                      [this] (IControl* pCaller)
                                                      {
                                                          pCaller->SetAnimation(
                                                            [this] (IControl* pCaller) {
                                                            auto progress = pCaller->GetAnimationProgress();
                                                            if(progress > 1.) {
                                                            pCaller->OnEndAnimation();
                                                            return;
                                                            }
                                                            dynamic_cast<IVectorBase*>(pCaller)->SetColor(kPR, IColor::LinearInterpolateBetween(COLOR_NEL_LUNADA_stop2, kPR, static_cast<float>(progress) ));
                                                            pCaller->SetDirty(false);
                                                          }
                                                            , 1000  ); //duration
                                for (int i=0; i<=3; i++) {
                                                          pCaller->GetUI()->
                                                          GetControlWithTag(kCtrlFluxDial1 + i)->
                                                          SetActionFunction ( [this, i] (IControl* pDialControl)
                                                                             {
                                                                              if (oscSender==nullptr) oscSender = std::make_unique<OSCSender>(); // localhost fallback
                                                                              if (oscSender!=nullptr){
                                                                                                       OscMessageWrite msg;
                                                                                                       std::string stem = "/flux" + std::to_string(i);
                                                                                                       msg.PushWord( stem.c_str() );
                                                                                                       msg.PushFloatArg(pDialControl->GetValue(0));
                                                                                                       msg.PushFloatArg(pDialControl->GetValue(1));
                                                                                                       oscSender->SendOSCMessage(msg);
                                                                                                      }
                                                             
                                                                              });
                                                            }
                                                        });
      }; //end layout lambda function
}
    
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
                                                                      oscSender =std::make_unique<OSCSender>(); //fallback to localhost
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



