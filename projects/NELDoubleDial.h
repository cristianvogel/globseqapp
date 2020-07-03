//
//  NELDoubleDial.hpp
//  APP
//
//  Created by Cristian Andres Vogel on 02/07/2020.
//

#pragma once

#include <stdio.h>
#include "IControl.h"

using namespace iplug;
using namespace iplug::igraphics;

class NELDoubleDial : public IKnobControlBase
{
public:
  NELDoubleDial(const IRECT& bounds, const std::initializer_list<int>& params, float a1 = -135.f, float a2 = 135.f, float aAnchor = -135.f)
  : IKnobControlBase(bounds)
  , mAngle1(a1)
  , mAngle2(a2)
  , mAnchorAngle(aAnchor)
  {
    int maxNTracks = static_cast<int>(params.size());
    SetNVals(maxNTracks);
    
    int valIdx = 0;
    for (auto param : params)
    {
      SetParamIdx(param, valIdx++);
    }
    
    SetValue(0.5, 0);
    SetValue(0.5, 1);
  }

  void Draw(IGraphics& p) override;
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override;
  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override;

    
private:
  double mMouseDragValue = 0;
  float mTrackToHandleDistance = 4.f;
  float mInnerPointerFrac = 0.1f;
  float mOuterPointerFrac = 1.f;
  float mPointerThickness = 2.5f;
  float mAngle1, mAngle2;
  float mTrackSize = 8.f;
  float mAnchorAngle; // for bipolar arc
};

