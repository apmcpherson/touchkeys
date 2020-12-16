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
 
  Types.h: basic types used throughout the program
*/

#pragma once

// For Windows, which defines min() and max() macros
#ifdef max
#undef max
#endif

#include <limits>
#include <cstdlib>
#include <cmath>
#include <utility>

#undef FIXED_POINT_TIME

// The following template specializations give the "missing" values for each kind of data that can be used in a Node.
// If an unknown type is added, its "missing" value is whatever comes back from the default constructor.  Generally speaking, new
// types should be added to this list as they are used

template<typename T>
struct missing_value {
	static const T missing() { return T(); }
	static const bool isMissing(T val) { return val == missing(); }
};

template<> struct missing_value<short> { 
	static const short missing() { return std::numeric_limits<short>::max(); } 
	static const bool isMissing(short val) { return (val == missing()); }
};
template<> struct missing_value<unsigned short> { 
	static const unsigned short missing() { return std::numeric_limits<unsigned short>::max(); } 
	static const bool isMissing(unsigned short val) { return (val == missing()); }
};
template<> struct missing_value<int> {	
	static const int missing() { return std::numeric_limits<int>::max(); } 
	static const bool isMissing(int val) { return (val == missing()); }
};
template<> struct missing_value<unsigned int> { 
	static const unsigned int missing() { return std::numeric_limits<unsigned int>::max(); } 
	static const bool isMissing(unsigned int val) { return (val == missing()); }
};
template<> struct missing_value<long> {	
	static const long missing() { return std::numeric_limits<long>::max(); } 
	static const bool isMissing(long val) { return (val == missing()); }
};
template<> struct missing_value<unsigned long> { 
	static const unsigned long missing() { return std::numeric_limits<unsigned long>::max(); } 
	static const bool isMissing(unsigned long val) { return (val == missing()); }
};
template<> struct missing_value<long long> { 
	static const long long missing() { return std::numeric_limits<long long>::max(); } 
	static const bool isMissing(long long val) { return (val == missing()); }
};
template<> struct missing_value<unsigned long long> { 
	static const unsigned long long missing() { return std::numeric_limits<unsigned long long>::max(); }
	static const bool isMissing(unsigned long long val) { return (val == missing()); }
};
template<> struct missing_value<float> { 
	static const float missing() { return std::numeric_limits<float>::quiet_NaN(); } 
	static const bool isMissing(float val) { 
#ifdef _MSC_VER
		return (_isnan(val) != 0);
#else
		return std::isnan(val); 
#endif
	}
};
template<> struct missing_value<double> { 
	static const double missing() { return std::numeric_limits<double>::quiet_NaN(); } 
	static const bool isMissing(double val) {
#ifdef _MSC_VER
		return (_isnan(val) != 0);
#else
		return std::isnan(val);  
#endif
	}
};
template<typename T1, typename T2>
struct missing_value<std::pair<T1,T2> > {
	static const std::pair<T1,T2> missing() { return std::pair<T1,T2>(missing_value<T1>::missing(), missing_value<T2>::missing()); }
	static const bool isMissing(std::pair<T1,T2> val) {
		return missing_value<T1>::isMissing(val.first) && missing_value<T2>::isMissing(val.second);
	}
};


// Globally-defined types: these types must be shared by all active units

#ifdef FIXED_POINT_TIME
typedef unsigned long long timestamp_type;
typedef long long timestamp_diff_type;

#define timestamp_abs(x) std::llabs(x)
#define ptime_to_timestamp(x) (x).total_microseconds()
#define timestamp_to_ptime(x) microseconds(x)
#define timestamp_to_milliseconds(x) ((x)/1000ULL)
#define microseconds_to_timestamp(x) (x)
#define milliseconds_to_timestamp(x) ((x)*1000ULL)
#define seconds_to_timestamp(x) ((x)*1000000ULL)

#else /* Floating point time */
typedef double timestamp_type;
typedef double timestamp_diff_type;

#define timestamp_abs(x) std::fabs(x)
#define ptime_to_timestamp(x) ((timestamp_type)(x).total_microseconds()/1000000.0)
#define timestamp_to_ptime(x) microseconds((x)*1000000.0)
#define timestamp_to_milliseconds(x) ((x)*1000.0)
#define microseconds_to_timestamp(x) ((double)(x)/1000000.0)
#define milliseconds_to_timestamp(x) ((double)(x)/1000.0)
#define seconds_to_timestamp(x) (x)

#endif /* FIXED_POINT_TIME */
