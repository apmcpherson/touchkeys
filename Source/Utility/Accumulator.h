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

  Accumulator.h: template class that accumulates (adds) samples coming into
  a given Node.
*/

#pragma once

#include "Node.h"
#include <exception>

/*
 * Accumulator
 *
 * Calculate the running sum of the last N points of a signal.  Unlike Integral, it does not
 * take into account the timestamps, and so is signficantly faster but less flexible.
 *
 * The output type of this class is a pair, the first of which indicates how many samples are
 * included in the accumulated result, and the second of which is the result itself.  This handles
 * transient startup conditions where all N samples are not yet available.
 *
 */

template<typename DataType, int N>
class Accumulator : public Node<std::pair<int, DataType> > {
public:
	typedef typename std::pair<int, DataType> return_type;
	typedef typename Node<return_type>::capacity_type capacity_type;
	//typedef typename Node<return_type>::size_type size_type;
	
	// ***** Constructors *****
		
	Accumulator(capacity_type capacity, Node<DataType>& input) : Node<return_type>(capacity), input_(input), samples_(N+1) {
		if(capacity <= N)			// Need to have at least N points in history to accumulate
			throw new std::bad_alloc();
		//std::cout << "Registering Accumulator\n";
		this->registerForTrigger(&input_);
		//std::cout << "Accumulator: this_source = " << (TriggerSource*)this << "this_dest = " << (TriggerDestination*)this << " input_ = " << &input_ << std::endl;
	}
				
	// Copy constructor
	Accumulator(Accumulator<DataType,N> const& obj) : Node<return_type>(obj), input_(obj.input_), samples_(obj.samples_) {
		this->registerForTrigger(&input_);
	}
	
	// ***** Modifiers *****
	//
	// Override this method to clear the samples_ buffer
	
	void clear() {
		Node<std::pair<int, DataType> >::clear();
		samples_.clear();
	}
	
	// ***** Evaluator *****
	//
	// This is called when the input gets a new data point.  Accumulate its value and store it in our buffer.
	// Storing it will also cause a trigger to be sent to anyone who's listening.
	
	void triggerReceived(TriggerSource* who, timestamp_type timestamp) {
		//std::cout << "Accumulator::triggerReceived\n";
		
		if(who != &input_)
			return;
		
		//std::cout << "Accumulator::triggerReceived2\n";		
		
		DataType newSample = input_.latest();
		samples_.push_back(newSample);		
		
		if(this->empty()) {
			this->insert(return_type(1, newSample), timestamp);
		}
		else {
			// Get the last point (both sample count and its accumulated value)
			return_type previousAccum = this->latest();
			
			// Add the current sample
			DataType accumulatedValue = newSample + previousAccum.second;
			int numPoints = previousAccum.first;
			
			// If necessary, subtract off the oldest sample, which by the size of samples_
			// is guaranteed to be its first point.
			if(samples_.full())
				accumulatedValue -= samples_.front();
			else
				numPoints++;
			
			this->insert(return_type(numPoints, accumulatedValue), timestamp);
		}
	}
	
	// Reset the integral to a given value at a given sample.  All samples
	// after this one are marked "missing" to force a recalculation of the integral next time
	// the value is requested.

	/*void reset(size_type index) {
		if(index < this->beginIndex() || index >= this->endIndex())
			return;
		this->rawValueAt(index) = std::pair<int, DataType>(0,DataType());
		size_type i = index + 1;
		while(i < this->endIndex())
			this->rawValueAt(index) = missing_value<std::pair<int, DataType> >::missing();
	}*/
	
private:
	Node<DataType>& input_;
	
	// Buffer holding the individual samples.  We need to be able to drop the last sample out of the
	// accumulated buffer, and including our own sample buffer means we don't need to rely on the
	// length of the input to store old samples.
	boost::circular_buffer<DataType> samples_;
};
