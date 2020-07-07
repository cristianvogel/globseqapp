//
//  GlobSeqHelpers.cpp
//  All
//
//  Created by Cristian Andres Vogel on 01/07/2020.
//

#include <iostream>
#include "GlobSeqHelpers.h"


//static decl work around see IPlugOsc.h
//std::unique_ptr<iplug::Timer> iplug::OSCInterface::mTimer;
//int iplug::OSCInterface::sInstances = 0;

//OSC Receiver
/*
void GlobSeqHelpers::OnOSCMessage(iplug::OscMessageRead& msg)
{
  //
}
*/

//remove spaces
std::string GlobSeqHelpers::chomp(std::string &str)
{
   str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
   return str;
}

