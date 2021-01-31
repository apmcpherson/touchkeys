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

  IIRFilter.h: template class handling an Nth-order IIR filter on data
  in a given Node.
*/

#pragma once


#include "Node.h"
#include <exception>
#include <vector>

/*
 * IIRFilterNode
 *
 * This template class performs IIR (infinite impulse response) filtering on incoming Node data.
 * It does not take into account timestamps so assumes the data is regularly sampled. The filter
 * coefficients can be specified and changed, and the operation is selectable between always
 * filtering on each new sample or only filtering on request. In the latter case, it will go back
 * and filter from the most recent available sample, assuming the signal starts from 0 if there is
 * any break in data between what was already calculated and what input data is now available.
 */

template<typename DataType>
class IIRFilterNode : public Node<DataType> {
public:
	typedef typename Node<DataType>::capacity_type capacity_type;
	//typedef typename Node<return_type>::size_type size_type;
	
	// ***** Constructors *****
    
	IIRFilterNode(capacity_type capacity, Node<DataType>& input) : Node<DataType>(capacity), input_(input),
      autoCalculate_(false), inputHistory_(0), outputHistory_(0), lastInputIndex_(0) {
	}
    
	// Copy constructor
	IIRFilterNode(IIRFilterNode<DataType> const& obj) : Node<DataType>(obj), input_(obj.input_), autoCalculate_(obj.autoCalculate_),
     aCoefficients_(obj.aCoefficients_), bCoefficients_(obj.bCoefficients_), lastInputIndex_(obj.lastInputIndex_) {
         if(obj.inputHistory_ != 0)
             inputHistory_ = new boost::circular_buffer<DataType>(*obj.inputHistory_);
         else
             inputHistory_ = 0;
         if(obj.outputHistory_ != 0)
             outputHistory_ = new boost::circular_buffer<DataType>(*obj.outputHistory_);
         else
             outputHistory_ = 0;
         if(autoCalculate_) {
             // Bring up to date and register for further updates
             calculate();
             this->registerForTrigger(&input_);
         }
	}
    
    // ***** Destructor *****
    ~IIRFilterNode() {
        if(inputHistory_ != 0)
            delete inputHistory_;
        if(outputHistory_ != 0)
            delete outputHistory_;
    }
	
	// ***** Modifiers *****
	//
	// Override this method to clear the input sample buffer
	
	void clear() {
		Node<DataType>::clear();
        clearInputOutputHistory();
	}
    
    // Switch whether calculations happen automatically or only upon request
    // If switching on, optionally specify how far back to calculate in bringing
    // the filter up to date.
    void setAutoCalculate(bool newAutoCalculate, int maximumLookback = -1) {
        if(autoCalculate_ && !newAutoCalculate)
            this->unregisterForTrigger(&input_);
        else if(!autoCalculate_ && newAutoCalculate) {
            // Bring the buffer up to date
            calculate(maximumLookback);
            this->registerForTrigger(&input_);
        }
        autoCalculate_ = newAutoCalculate;
    }
    
    // Set the coefficients of the filter and allocate the proper buffer
    // to hold past inputs. Optional last argument specifies whether to
    // clear the past sample history or not (defaults to clearing it).
    // If filter lengths are different, the buffer is always cleared.
    void setCoefficients(std::vector<DataType> const& bCoeffs,
                         std::vector<DataType> const& aCoeffs,
                         bool clearBuffer = true) {
        if(bCoeffs.empty()) // Can't have an empty feedforward coefficient set
            return;
        bool shouldClear = clearBuffer;
        aCoefficients_ = aCoeffs;
        bCoefficients_ = bCoeffs;
        
        if(inputHistory_ == nullptr) {
            inputHistory_ = new boost::circular_buffer<DataType>(bCoeffs.size());
            shouldClear = true;
        }
        else if(bCoeffs.size() != inputHistory_->capacity()) {
            inputHistory_->set_capacity(bCoeffs.size());
            shouldClear = true;
        }
        
        if(outputHistory_ == nullptr) {
            outputHistory_ = new boost::circular_buffer<DataType>(aCoeffs.size());
            shouldClear = true;
        }
        else if(aCoeffs.size() != outputHistory_->capacity()) {
            outputHistory_->set_capacity(aCoeffs.size());
            shouldClear = true;
        }
        
        if(shouldClear)
            clearInputOutputHistory();
    }
    
    // If not automatically calculating, bring the samples up to date by
    // processing any available input that we have not yet seen. Returns
    // the most recent output. Optional argument specifies how far back
    // to look before starting fresh (if more samples have elapsed since
    // last calculation).
    typename Node<DataType>::return_value_type calculate(int maximumLookback = -1) {
        typename Node<DataType>::size_type index = lastInputIndex_;
        
        if(maximumLookback >= 0 && index < input_.endIndex() - 1 - maximumLookback) {
            //std::cout << "IIRFilterNode: clearing history at index " << index << std::endl;
            // More samples gone by than we want to calculate... clear input
            clearInputOutputHistory();
            index = input_.endIndex() - 1 - maximumLookback;
            if(index < input_.beginIndex())
                index = input_.beginIndex();
        }
        else if(index < input_.beginIndex()) {
            // More samples gone by than are now available... clear input
            //std::cout << "IIRFilterNode: clearing history at index " << index << std::endl;
            clearInputOutputHistory();
            index = input_.beginIndex();
        }
        while(index < input_.endIndex()) {
            processOneSample(input_[index], input_.timestampAt(index));
            index++;
        }
        
        lastInputIndex_ = index;
        if(!this->empty())
            return this->latest();
        //std::cout << "IIRFilterNode: empty\n";
        return missing_value<DataType>::missing();
    }

	// ***** Evaluator *****
	//
	// This is called when the input gets a new data point.  Accumulate its value and store it in our buffer.
	// Storing it will also cause a trigger to be sent to anyone who's listening.
	
	void triggerReceived(TriggerSource* who, timestamp_type timestamp) {
		if(who != &input_ || !autoCalculate_)
			return;
        
        processOneSample(input_.latest(), timestamp);
	}
	
private:
    // ***** Internal Methods *****
    // Run the filter once with a new sample. Put the result into the
    // end of the buffer.
    void processOneSample(DataType const& sample, timestamp_type timestamp) {
        if(!bCoefficients_.empty()) {
            // Always need at least one feedforward coefficient
            DataType result = bCoefficients_[0] * sample;
            typename boost::circular_buffer<DataType>::reverse_iterator rit = inputHistory_->rbegin();
            
            // Feedforward part
            for(int i = 1; i < bCoefficients_.size() && rit != inputHistory_->rend(); i++) {
                result += *rit * bCoefficients_[i];
                rit++;
            }
            // Feedback part
            rit = outputHistory_->rbegin();
            for(int i = 0; i < aCoefficients_.size() && rit != outputHistory_->rend(); i++) {
                result -= *rit * aCoefficients_[i];
                rit++;
            }
            
            // Update input history and put output in our buffer
            inputHistory_->push_back(sample);
            outputHistory_->push_back(result);
            this->insert(result, timestamp);
        }
        else {
            // Pass through when no coefficients present
            this->insert(sample, timestamp);
        }
    }
    
    // Clear the recent history of input/output data and fill it with zeros
    void clearInputOutputHistory() {
        if(inputHistory_ != 0) {
            inputHistory_->clear();
            while(!inputHistory_->full())
                inputHistory_->push_back(DataType());
        }
        if(outputHistory_ != 0) {
            outputHistory_->clear();
            while(!outputHistory_->full())
                outputHistory_->push_back(DataType());
        }
    }
    
    
    // ***** Member Variables *****
    
	Node<DataType>& input_;
	bool autoCalculate_;        // Whether we're automatically calculating new output values

    // Variables below are for filter calculation. We need to hold the past input samples
    // ourselves because we can't consistently count on enough samples in the source buffer.
    // Likewise, we need to hold past output samples, even though we have our own buffer, because
    // when we clear the buffer for new calculations we don't want to lose what we've previously
    // calculated.
    boost::circular_buffer<DataType>* inputHistory_;
    boost::circular_buffer<DataType>* outputHistory_;
    std::vector<DataType> aCoefficients_, bCoefficients_;
    typename Node<DataType>::size_type lastInputIndex_;              // Where in the input buffer we had the last sample
};

// ***** Static Filter Design Methods *****

// These methods calculate specific coefficients and store them in the provided
// A and B vectors. These will only work for double/float datatypes or others that
// can be multiplied with them. Details: http://freeverb3.sourceforge.net/iir_filter.shtml

void designFirstOrderLowpass(std::vector<double>& bCoeffs, std::vector<double>& aCoeffs,
                                    double cutoffFrequency, double sampleFrequency);

void designFirstOrderHighpass(std::vector<double>& bCoeffs, std::vector<double>& aCoeffs,
                              double cutoffFrequency, double sampleFrequency);

void designSecondOrderLowpass(std::vector<double>& bCoeffs, std::vector<double>& aCoeffs,
                              double cutoffFrequency, double q, double sampleFrequency);

void designSecondOrderHighpass(std::vector<double>& bCoeffs, std::vector<double>& aCoeffs,
                               double cutoffFrequency, double q, double sampleFrequency);

void designSecondOrderBandpass(std::vector<double>& bCoeffs, std::vector<double>& aCoeffs,
                               double cutoffFrequency, double q, double sampleFrequency);
