#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugOSC.h"
#include "GlobSeqHelpers.h"
#include "NELDoubleDial.h"

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

enum EStatusMessages
{
  kMsgScanning = 0,
  kMsgConnected,
  kNumStatusMessages
};


using namespace iplug;
using namespace igraphics;

class GlobSeqPlugIn final : public Plugin
{
public:
  GlobSeqPlugIn(const InstanceInfo& info);
  ~GlobSeqPlugIn();

  std::string beSlimeName = "";
  std::vector<std::string> cnsl =
  {
    "⚇ BeSlime?",
    "⚉ "
  };
  std::string beSlimeIP = "⋯";
  std::string consoleText = "";

  
  WDL_String senderNetworkInfo;

  std::unique_ptr<OSCSender> oscSender;
  std::atomic_bool beSlimeConnected {false};
  std::mutex mtx;           // mutex for critical section

  
  IText consoleFont;
  GlobSeqHelpers* gsh = new GlobSeqHelpers();
  
private:
  void launchNetworkingThreads();
  
#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif
};
