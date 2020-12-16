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
 
  PianoTypes.h: some useful constants mainly relevant to continuous key angle
*/

#pragma once

#include "../Utility/Types.h"

#undef FIXED_POINT_PIANO_SAMPLES

// Data types.  Allow for floating-point (more flexible) or fixed-point (faster) arithmetic
// on piano key positions
#ifdef FIXED_POINT_PIANO_SAMPLES
typedef int key_position;
typedef int key_velocity;
#define scale_key_position(x) 4096*(key_position)(x)
#define key_position_to_float(x) ((float)x/4096.0)
#define key_abs(x) abs(x)
#define calculate_key_velocity(dpos, dt) (key_velocity)((65536*dpos)/(key_position)dt)
#define scale_key_velocity(x) (65536/4096)*(key_velocity)(x) // FIXME: TEST THIS!
#else
typedef float key_position;
typedef float key_velocity;
#define scale_key_position(x) (key_position)(x)
#define key_position_to_float(x) (x)
#define key_abs(x) fabsf(x)
#define calculate_key_velocity(dpos, dt) (key_velocity)(dpos/(key_position)dt)
#define scale_key_velocity(x) (key_velocity)(x)
#endif /* FIXED_POINT_PIANO_SAMPLES */
