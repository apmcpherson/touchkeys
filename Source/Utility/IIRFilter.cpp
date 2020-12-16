/*
  TouchKeys: multi-touch musical keyboard control software
  Copyright (c) 2013 Andrew McPherson

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
  =====================================================================

  IIRFilter.cpp: template class handling an Nth-order IIR filter on data
  in a given Node.
*/

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include "IIRFilter.h"

// These are static functions to design IIR filters specifically for floating-point datatypes.
// vector<double> and be converted to another type at the end if needed.

void designFirstOrderLowpass(std::vector<double>& bCoeffs, std::vector<double>& aCoeffs,
                                    double cutoffFrequency, double sampleFrequency) {
    bCoeffs.clear();
    aCoeffs.clear();
    
    double omega = tan(M_PI * cutoffFrequency / sampleFrequency);
    double n = 1.0 / (1.0 + omega);
    
    bCoeffs.push_back(omega * n);       // B0
    bCoeffs.push_back(omega * n);       // B1
    aCoeffs.push_back((omega - 1) * n); // A1
}

void designFirstOrderHighpass(std::vector<double>& bCoeffs, std::vector<double>& aCoeffs,
                             double cutoffFrequency, double sampleFrequency) {
    bCoeffs.clear();
    aCoeffs.clear();
    
    double omega = tan(M_PI * cutoffFrequency / sampleFrequency);
    double n = 1.0 / (1.0 + omega);
    
    bCoeffs.push_back(n);               // B0
    bCoeffs.push_back(-n);              // B1
    aCoeffs.push_back((omega - 1) * n); // A1
}

void designSecondOrderLowpass(std::vector<double>& bCoeffs, std::vector<double>& aCoeffs,
                             double cutoffFrequency, double q, double sampleFrequency) {
    bCoeffs.clear();
    aCoeffs.clear();
    
    double omega = tan(M_PI * cutoffFrequency / sampleFrequency);
    double n = 1.0 / (omega*omega + omega/q + 1.0);
    double b0 = n * omega * omega;
    
    bCoeffs.push_back(b0);       // B0
    bCoeffs.push_back(2.0 * b0); // B1
    bCoeffs.push_back(b0);       // B2
    aCoeffs.push_back(2.0 * n * (omega * omega - 1.0));   // A1
    aCoeffs.push_back(n * (omega * omega - omega / q + 1.0));
}

void designSecondOrderHighpass(std::vector<double>& bCoeffs, std::vector<double>& aCoeffs,
                              double cutoffFrequency, double q, double sampleFrequency) {
    bCoeffs.clear();
    aCoeffs.clear();
    
    double omega = tan(M_PI * cutoffFrequency / sampleFrequency);
    double n = 1.0 / (omega*omega + omega/q + 1.0);
    
    bCoeffs.push_back(n);        // B0
    bCoeffs.push_back(-2.0 * n); // B1
    bCoeffs.push_back(n);        // B2
    aCoeffs.push_back(2.0 * n * (omega * omega - 1.0));   // A1
    aCoeffs.push_back(n * (omega * omega - omega / q + 1.0));
}

void designSecondOrderBandpass(std::vector<double>& bCoeffs, std::vector<double>& aCoeffs,
                               double cutoffFrequency, double q, double sampleFrequency) {
    bCoeffs.clear();
    aCoeffs.clear();
    
    double omega = tan(M_PI * cutoffFrequency / sampleFrequency);
    double n = 1.0 / (omega*omega + omega/q + 1.0);
    double b0 = n * omega / q;
    bCoeffs.push_back(b0);       // B0
    bCoeffs.push_back(0.0);      // B1
    bCoeffs.push_back(-b0);      // B2
    aCoeffs.push_back(2.0 * n * (omega * omega - 1.0));   // A1
    aCoeffs.push_back(n * (omega * omega - omega / q + 1.0));
}