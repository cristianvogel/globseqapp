//
//  GlobSeqHelpers.cpp
//  All
//
//  Created by Cristian Andres Vogel on 01/07/2020.
//

#include <iostream>
#include "GlobSeqHelpers.h"


//remove spaces
std::string GlobSeqHelpers::chomp(std::string &str)
{
   str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
   return str;
}

