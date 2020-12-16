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

  LineSegment.h: template class implementing a line segment over time.
*/

#pragma once

#include <iostream>
#include <list>
#include <boost/function.hpp>
#include "Types.h"

/*
 * LineSegment
 *
 * This template class implements a line (or curve) segment. Given start and end times and
 * start and end values, this class calculates the current value based on the given timestamp.
 * Options are available to freeze at the final value or to extrapolate from it.
 * 
 * One assumption that is made with this class is that calls are always made in monotonically
 * non-decreasing order of timestamp. Evaluating the value of the segment may implicitly update 
 * its state so out-of-order evaluations could return unexpected results.
 */


template<typename DataType>
class LineSegment {
public:
    typedef boost::function<DataType const& (DataType const&)> warp_function;
    
    struct {
        timestamp_type time;        // Time to achieve a given value
        DataType       value;       // Value that should be achieved
        warp_function  warp;        // How to warp the data getting there. FIXME: null?
        bool           jump;        // Whether to do an immediate jump
    } Endpoint;
    
public:
	// ***** Constructors *****

    // Default constructor with default starting value
    LineSegment() : lastEvaluatedTimestamp_(0), lastEvaluatedValue_() {
        
    }
    
    // Constructor with arbitrary starting value
    LineSegment(DataType const& startingValue) : lastEvaluatedTimestamp_(0),
      lastEvaluatedValue_(startingValue) {
        
    }
    
	// Copy constructor
    LineSegment(LineSegment<DataType> const& obj) : 
      lastEvaluatedTimestamp_(obj.lastEvaluatedTimestamp_),
      lastEvaluatedValue_(obj.lastEvaluatedValue_) {
        
    }
        
    // ***** Destructor *****
    ~LineSegment() {

    }
	
	// ***** Modifiers *****
    // Add a new segment from a given time to a given time. The "from" value is implicitly
    // calculated based on the current state of the segment.
    void addSegment(timestamp_type const fromTimestamp, timestamp_type const toTimestamp, DataType const& toValue) {
        
    }
    
    // As above, but use a non-linear (warped) transfer function to get from one end to the other.
    // The function should take in one value between 0 and 1 (or their DataType equivalents) and return
    // a value between 0 and 1 (or their DataType equivalents). Scaling will be done internally.
    void addSegment(timestamp_type const fromTimestamp, timestamp_type const toTimestamp, DataType const& toValue,
                    warp_function& warp) {
        
    }
    
    // Freeze the value starting at the indicated timestamp (immediately if no argument is given)
    void hold(timestamp_type const timestamp = 0) {
        if(timestamp == 0) {
            segments_.clear();          // Hold at whatever last evaluation returned
        }
        else {
            evaluate(timestamp);        // Recalculate value for this time, then hold
            segments_.clear();
        }
    }
    
    // Jump to a given value at the indicated timestamp (immediately if no argument is given)
    void jump(timestamp_type const timestamp = 0, DataType const& toValue) {
        if(timestamp == 0) {
            lastEvaluatedValue_ = toValue;
            segments_.clear();
        }
        else {
            // Jump to a value at a given time
            Endpoint newEndpoint;
            
            newEndpoint.time = timestamp;
            newEndpoint.value = toValue;
            newEndpoint.jump = true;
            
            if(segments_.empty())
                segments_.push_back(newEndpoint);
            else {
                auto it = segments_.begin();
                
                // Look for any elements in the list that have a later timestamp.
                // Remove them and make this jump the last element.
                while(it != segments_.end()) {
                    if(it->time >= timestamp) {
                        segments_.erase(it, segments_.end());
                        break;
                    }
                    it++;
                }
                
                segments_.push_back(newEndpoint);
            }
        }
    }
    
    // Reset state to defaults, including times
    void reset() {
        lastEvaluatedValue_ = DataType();
        lastEvaluatedTimestamp_ = 0;
        segments_.clear();
    }
    
	// ***** Evaluator *****
    
    // Return the current value of the segment based on current timestamp
    DataType const& evaluate(timestamp_type const timestamp) {
        // Check that the current timestamp is later than the previous one. If
        // earlier, print a warning. If identical, return the last value identically.
        // If there are no further changes planned, likewise return the last value.
        if(timestamp < lastEvaluatedTimestamp_) {
            std::cout << "Warning: LineSegment::evaluate() called with timestamp " << timestamp << " earlier than previous " << lastEvaluatedTimestamp_ << std::endl;
            return lastEvaluatedValue_;
        }
        if(timestamp == lastEvaluatedValue_ || !changesRemaining_)
            return lastEvaluatedValue_;
        
        // TODO: evaluate
    }
    
    // Return whether all in-progress segments have finished. Call this after evaluate().
    bool finished() {
        return segments_.empty();
    }

private:
    // ***** Internal Methods *****

    
    // ***** Member Variables *****
    timestamp_type  lastEvaluatedTimestamp_;    // When evaluate() was last called
    DataType        lastEvaluatedValue_;        // What evaluate() last returned
    std::list<Endpoint> segments_;              // List of segment times and values
};
