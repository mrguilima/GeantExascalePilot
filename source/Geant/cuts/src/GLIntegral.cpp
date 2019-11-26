//===------------------ GeantX --------------------------------------------===//
//
// Geant Exascale Pilot
//
// For the licensing terms see LICENSE file.
// For the list of contributors see CREDITS file.
// Copyright (C) 2019, Geant Exascale Pilot team,  All rights reserved.
//===----------------------------------------------------------------------===//
/**
 * @file Geant/cuts/src/GLIntegral.cpp
 * @brief Originated in the GeantV project
 */
//===----------------------------------------------------------------------===//

#include "Geant/cuts/GLIntegral.hpp"

#include "Geant/core/PhysicalConstants.hpp"
#include "Geant/core/math_wrappers.hpp"

#include <cmath>

namespace geantx {
inline namespace GEANT_IMPL_NAMESPACE {

GLIntegral::GLIntegral(int npoints, double xmin, double xmax)
{
  fNPoints = npoints;
  fXmin    = xmin;
  fXmax    = xmax;
  fWeights.resize(fNPoints);
  fAbscissas.resize(fNPoints);
  SetParameters();
}

void GLIntegral::SetParameters()
{
  using geantx::units::kPi;
  const double epsilon = 1.0e-13;
  double xm, xl, z, z1, p1, p2, p3, pp;
  int m = (int)((fNPoints + 1.) / 2.);
  xm    = 0.5 * (fXmax + fXmin);
  xl    = 0.5 * (fXmax - fXmin);
  for (int i = 1; i <= m; ++i) {
    z = Math::Cos(kPi * (i - 0.25) / (fNPoints + 0.5));
    do {
      p1 = 1.0;
      p2 = 0.0;
      for (int j = 1; j <= fNPoints; ++j) {
        p3 = p2;
        p2 = p1;
        p1 = ((2.0 * j - 1.0) * z * p2 - (j - 1.0) * p3) / (j);
      }
      pp = fNPoints * (z * p1 - p2) / (z * z - 1.0);
      z1 = z;
      z  = z1 - p1 / pp;
    } while (std::fabs(z - z1) > epsilon);
    fAbscissas[i - 1]                = xm - xl * z;
    fAbscissas[fNPoints + 1 - i - 1] = xm + xl * z;
    fWeights[i - 1]                  = 2.0 * xl / ((1.0 - z * z) * pp * pp);
    fWeights[fNPoints + 1 - i - 1]   = fWeights[i - 1];
  }
}

} // namespace GEANT_IMPL_NAMESPACE
} // namespace geantx
