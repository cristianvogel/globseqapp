//
//  NELDoubleDial.cpp
//  APP
//
//  Created by Cristian Andres Vogel on 02/07/2020.
//

#include "NELDoubleDial.h"
#include <algorithm>

void NELDoubleDial::Draw(IGraphics& g)  {

  float radius;
  
  if(mRECT.W() > mRECT.H())
    radius = (mRECT.H()/2.f);
  else
    radius = (mRECT.W()/2.f);
  
  const float cx = mRECT.MW(), cy = mRECT.MH();
  
  radius -= (mTrackSize/2.f);
  
  float angle = mAngle1 + (static_cast<float>(GetValue(0)) * (mAngle2 - mAngle1));
 
  interp = IColor::LinearInterpolateBetween(COLOR_NEL_LUNADA_stop2, COLOR_NEL_LUNADA_stop3, static_cast<float>( fmin(1.0f, GetValue(0)*2.0f)) );
  
  g.DrawCircle(COLOR_WHITE, cx, cy, radius,nullptr, 0.5f);
  g.DrawArc(IColor::LinearInterpolateBetween(COLOR_NEL_LUNADA_stop1, interp, static_cast<float>(GetValue(0))),
            cx, cy, radius,
            angle >= mAnchorAngle ? mAnchorAngle : mAnchorAngle - (mAnchorAngle - angle),
            angle >= mAnchorAngle ? angle : mAnchorAngle, &mBlend, mTrackSize);
  
  radius -= mTrackSize;
  
  angle = mAngle1 + (static_cast<float>(GetValue(1)) * (mAngle2 - mAngle1));
  
  g.DrawCircle(COLOR_GRAY, cx, cy, radius,nullptr, 0.5f);
  g.DrawArc(IColor::LinearInterpolateBetween(COLOR_NEL_LUNADA_stop1, COLOR_NEL_LUNADA_stop2, static_cast<float>(GetValue(1))),
             cx, cy, radius,
            angle >= mAnchorAngle ? mAnchorAngle : mAnchorAngle - (mAnchorAngle - angle),
            angle >= mAnchorAngle ? angle : mAnchorAngle, &mBlend, mTrackSize);
}

void NELDoubleDial::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod)
{
  double gearing = IsFineControl(mod, false) ? mGearing * 10.0 : mGearing;
  
  IRECT dragBounds = GetKnobDragBounds();
  
  if (mDirection == EDirection::Vertical)
    mMouseDragValue += static_cast<double>(dY / static_cast<double>(dragBounds.T - dragBounds.B) / gearing);
  else
    mMouseDragValue += static_cast<double>(dX / static_cast<double>(dragBounds.R - dragBounds.L) / gearing);
  
  mMouseDragValue = Clip(mMouseDragValue, 0., 1.);
  
  double v = mMouseDragValue;
  //    const IParam* pParam = GetParam();
  //
  //    if (pParam && pParam->GetStepped() && pParam->GetStep() > 0)
  //    {
  //      const double range = pParam->GetRange();
  //
  //      if (range > 0.)
  //      {
  //        double l, h;
  //        pParam->GetBounds(l, h);
  //
  //        v = l + mMouseDragValue * range;
  //        v = v - std::fmod(v, pParam->GetStep());
  //        v -= l;
  //        v /= range;
  //      }
  //    }
  
  SetValue(v, mod.C ? 0 : 1 /*needs to change depending on arc clicked */); // TODO: modify
  SetDirty();
}
