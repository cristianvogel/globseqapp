#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugOSC.h"
#include "GlobSeqHelpers.h"
#include "NELDoubleDial.h"
#include <atomic>

const int kNumPresets = 1;

#define NBR_DUALDIALS 8

enum EParams
{
  kNetstatus = 0,
  kReScan,
  kDualDialInner,
  kDualDialOuter = kDualDialInner + NBR_DUALDIALS,
  kNumParams = kDualDialOuter + NBR_DUALDIALS
};

enum EControlTags
{
  kCtrlNetStatus = 0,
  kCtrlReScan,
  kCtrlFluxDial,
  kNumCtrlTags = kCtrlFluxDial + NBR_DUALDIALS
};

enum EControlDialTags
{
  kCtrlFluxDialInner = 0,
  kCtrlFluxDialOuter = kCtrlFluxDialInner + NBR_DUALDIALS,
  kNumCtrlFluxDials = kCtrlFluxDialOuter + NBR_DUALDIALS
};

enum EStatusMessages
{
  kMsgScanning = 0,
  kMsgConnected,
  kNumStatusMessages
};

class GlobSeqPlugIn final : public Plugin
{
public:
  GlobSeqPlugIn(const InstanceInfo& info);
  ~GlobSeqPlugIn();

  std::string beSlimeName = "";
  std::vector<std::string> cnsl =
  {
    "⚇ localhost",
    "⚉ "
  };
  std::string beSlimeIP = "⋯";
  std::string consoleText = "";
  WDL_String senderNetworkInfo;
  std::unique_ptr<OSCSender> oscSender;
  std::atomic_bool beSlimeConnected {false};
  std::mutex mtx;           // mutex for critical section in network thread
  
  IText consoleFont;
  std::unique_ptr<GlobSeqHelpers> gsh = std::make_unique<GlobSeqHelpers>();
  
private:
  void launchNetworkingThreads();
  
#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif
};
