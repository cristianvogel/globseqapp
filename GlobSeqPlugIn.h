#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugOSC.h"
#include <atomic>

const int kNumPresets = 1;

enum EParams
{
  kBPM = 0,
  kNetstatus,
  kReScan,
  kNumParams
};

enum EControlTags
{
  kCtrlTagBPM = 0,
  kCtrlNetStatus,
  kCtrlReScan,
  kNumCtrlTags
};

using namespace iplug;
using namespace igraphics;

class GlobSeqPlugIn final : public Plugin
{
public:
  GlobSeqPlugIn(const InstanceInfo& info);
  ~GlobSeqPlugIn();

  std::string beSlimeName = "⚆ Looking for beslime...";
  std::string beSlimeIP = "";
  
  WDL_String senderNetworkInfo;

  std::unique_ptr<OSCSender> oscSender;
  IText consoleFont;
  
private:
  void launchNetworkingThreads();
  std::atomic_bool stop_scan_thread {false};
  
#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif
};
