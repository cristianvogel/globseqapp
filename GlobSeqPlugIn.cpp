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
  
  InitParamRange(kDualDialInner, kDualDialInner + NBR_DUALDIALS - 1, 1, "Dual Dial %i", 0, 0., 1., 0,"%",0,"Inner Value");
  InitParamRange(kDualDialOuter, kDualDialOuter + NBR_DUALDIALS - 1, 1, "Dual Dial %i", 0, 0., 1., 0,"%",0,"Outer Value");

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
              12.f,
              COLOR_LIGHT_GRAY,
              "Menlo",
              EAlign::Center,
              EVAlign::Middle,
              0.f,
              DEFAULT_TEXTENTRY_BGCOLOR,
              DEFAULT_TEXTENTRY_FGCOLOR
              ) // Label text
      };
      
#pragma mark mainCanvas
      // main app GUI IRECT
      const IRECT b = pGraphics->GetBounds().GetScaledAboutCentre(0.95f);
      const IRECT consoleBounds = b.GetFromBottom( 12.f ).GetGridCell(1, 1, 3);

#pragma mark console text
      //▼ small logging console output status update text
      consoleFont = IText ( 12.f, "Menlo").WithFGColor(COLOR_NEL_TUNGSTEN_FGBlend);
      pGraphics->AttachControl(new ITextControl(consoleBounds, consoleText.c_str(), consoleFont, false), kCtrlNetStatus);
      pGraphics->AttachControl(new ILambdaControl( consoleBounds,
                                                             [this](ILambdaControl* pCaller, IGraphics& g, IRECT& rect)
                                                               {
                                                                 ITextControl* cnsl = dynamic_cast<ITextControl*>(g.GetControlWithTag(kCtrlNetStatus));
                                                                 cnsl->SetStr(consoleText.c_str());
                                                                 cnsl->SetDirty();
                                                               },
                                                             DEFAULT_ANIMATION_DURATION, true /*loop*/, false /*start imediately*/));
#pragma mark dual dials
      //▼ rows of dual concentric dials with two paramIdx
      for (int d=0; d<NBR_DUALDIALS; d++) {

        const IRECT dualDialBounds = b.GetGridCell(0, 0, 4, 1).SubRectHorizontal(NBR_DUALDIALS, d).FracRect(EDirection::Horizontal, 0.75f);
      pGraphics->AttachControl
                (new NELDoubleDial(
                                   dualDialBounds,
                                   {kDualDialInner + d, kDualDialOuter + d}), kCtrlFluxDial+d
                 );
      }
      
#pragma mark network button / OSC value output
      //▼ Network button/status attaches OSC ActionFunction lambda to concentric dials
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
                                                            dynamic_cast<IVectorBase*>(pCaller)->
                                                              SetColor(kPR, IColor::LinearInterpolateBetween(COLOR_NEL_LUNADA_stop2, kPR, static_cast<float>(progress)));
                                                            pCaller->SetDirty(false);
                                                          }
                                                            , 1000  ); //click flash duration
                                for (int i=0; i<NBR_DUALDIALS; i++) {
                                                IControl* pDialLoop = pCaller->GetUI()->GetControlWithTag(kCtrlFluxDial + i);
                                                          pDialLoop->SetActionFunction ( [this, i] (IControl* pDialLambda)
                                                                             {
                                                                              if (!oscSender) oscSender = std::make_unique<OSCSender>(); // localhost fallback
                                                                              if (oscSender) {
//todo: avoid crash when connection is lost.
//Needs IP change method implementation in iPlugOSC.h
                                                                                               OscMessageWrite msg;
                                                                                               std::string stem = "/dualDial/" + std::to_string(i);
                                                                                               msg.PushWord( stem.c_str() );
                                                                                               msg.PushFloatArg(pDialLambda->GetValue(0));
                                                                                               msg.PushFloatArg(pDialLambda->GetValue(1));
                                                                                               oscSender->SendOSCMessage(msg);
                                                                                              }
                                                                              });
                                                          pDialLoop->SetDirty();
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
                                                   //extract beslime name
                                                   std::stringstream ss(bytes);
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
                                                               std::stringstream ssip(bytes);
                                                               std::string ipLine;
                                                              
                                                               while(std::getline(ssip,ipLine,'\n'))
                                                               {
                                                                 if(ipLine.find("Rmv") != std::string::npos)
                                                                 {
                                                                    mtx.lock();
                                                                      oscSender.reset();
                                                                      oscSender = std::make_unique<OSCSender>(); //fallback to localhost
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



