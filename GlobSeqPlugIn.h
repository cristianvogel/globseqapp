#pragma once

#include "IPlug_include_in_plug_hdr.h"


const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class GlobSeqPlugIn final : public Plugin
{
public:
  GlobSeqPlugIn(const InstanceInfo& info);
  ~GlobSeqPlugIn();
  
  std::string beSlimeName = "";
  std::string beSlimeIP = "";
  
#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif
};
