#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugOSC.h"

const int kNumPresets = 1;

enum EParams
{
  kBPM = 0,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class GlobSeqPlugIn final : public Plugin, public OSCSender
{
public:
  GlobSeqPlugIn(const InstanceInfo& info);
  ~GlobSeqPlugIn();
  
  
  std::string beSlimeName = "◌ Looking for beslime...";
  std::string beSlimeIP = "";
  
  WDL_String senderNetworkInfo;
  
  //I tried creating an OSCSender object on the heap
  //but i don't understand how to change its sending destination
  OSCSender *oscSender = new OSCSender();
  
#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif
};
