//
//  GlobSeqHelpers.hpp
//  All
//
//  Created by Cristian Andres Vogel on 01/07/2020.
//
#pragma once

class GlobSeqHelpers //: public iplug::OSCReciever
{
public:
  
  GlobSeqHelpers() //: iplug::OSCReciever(8080)
  {}
  
  ~GlobSeqHelpers()
  {}
  
  //virtual void OnOSCMessage(iplug::OscMessageRead& msg) override;
 
  std::string chomp(std::string &str);
};

