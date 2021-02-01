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

  Node.h: template class which wraps a circular buffer, providing a collection
  of methods for inserting, accessing and iterating over data (including
  interpolation). Also provides trigger functionality to notify listeners when
  new objects are inserted. Unlike conventional containers, indices always increment
  over time, even though only the last N objects will be held in the buffer. The
  beginning index will therefore be greater than 0 in many cases, but the index of
  a particular item will never change, even if the item itself eventually is dropped
  from the buffer.
*/

#pragma once

#include "Trigger.h"
#include <boost/circular_buffer.hpp>
#include <boost/lambda/lambda.hpp>
#include <cmath>
#include <stdint.h>


template<typename OutputType> class NodeNonInterpolating;
template<typename OutputType> class Node;

/*
 * NodeIterator
 *
 * Abstract class that implements common functionality for all types of Sources and Filters
 * Modeled on (and contains) boost::cb_details::iterator
 * See boost/circular_buffer/details.hpp for more information
 *
 */

 // Custom iterator type to move through the Node buffer
template <class OutputType, class Traits, class NonConstTraits>
struct NodeIterator :
	public boost::iterator<
	std::random_access_iterator_tag,
	typename Traits::value_type,
	typename Traits::difference_type,
	typename Traits::pointer,
	typename Traits::reference>
{
	typedef boost::iterator<
		std::random_access_iterator_tag,
		typename Traits::value_type,
		typename Traits::difference_type,
		typename Traits::pointer,
		typename Traits::reference> base_iterator;

	typedef NodeNonInterpolating<OutputType> Buff;

	typedef NodeIterator<Buff, typename Traits::nonconst_self, NonConstTraits> nonconst_self;
	typedef typename boost::cb_details::iterator<boost::circular_buffer<OutputType>, NonConstTraits> cb_iterator;

	typedef typename base_iterator::value_type value_type;
	typedef typename base_iterator::pointer pointer;
	typedef typename base_iterator::reference reference;
	typedef typename Traits::size_type size_type;
	typedef typename base_iterator::difference_type difference_type;

	// ***** Member Variables *****

	// Pointer to the Node object
	Buff* m_buff;

	// Base (non-const) iterator to the circular buffer
	cb_iterator m_cb_it;

	// ***** Constructors *****

	// Default constructor
	NodeIterator() : m_buff( 0 ), m_cb_it( cb_iterator() ) {}

	// Copy constructor
	NodeIterator( const nonconst_self& it ) : m_cb_it( it.m_cb_it ) {}

	// Constructor based on a circular_buffer iterator
	NodeIterator( Buff* cb, const cb_iterator cb_it ) : m_buff( cb ), m_cb_it( cb_it ) {}

	// ***** Operators *****
	//
	// Modeled on boost::cb_details::iterator (boost/circular_buffer/details.hpp)

	NodeIterator& operator = ( const NodeIterator& it ) {
		if( this == &it )
			return *this;
		m_buff = it.m_buff;
		m_cb_it = it.m_cb_it;
		return *this;
	}

	// Dereferencing operator.  We change the behavior here to evaluate missing values as needed.
	// Note that this requires m_cb_it to be a non_const type, even if we are a const iterator.

	reference operator * () const {
		return *m_cb_it;
		//reference val = *m_cb_it;
		//if(!missing_value<OutputType>::isMissing(val))
		//	return val;
		//return (*m_cb_it = m_buff->evaluate(index()));
	}

	pointer operator -> () const { return &( operator*() ); }

	template <class Traits0, class Traits1>
	difference_type operator - ( const NodeIterator<OutputType, Traits0, Traits1>& it ) const { return m_cb_it - it.m_cb_it; }

	NodeIterator& operator ++ () {			// ++it
		++m_cb_it;
		return *this;
	}
	NodeIterator operator ++ ( int ) {		// it++
		NodeIterator<OutputType, Traits, NonConstTraits> tmp = *this;
		++m_cb_it;
		return tmp;
	}
	NodeIterator& operator -- () {			// --it
		--m_cb_it;
		return *this;
	}
	NodeIterator operator -- ( int ) {		// it--
		NodeIterator<OutputType, Traits, NonConstTraits> tmp = *this;
		m_cb_it--;
		return tmp;
	}
	NodeIterator& operator += ( difference_type n ) {		// it += n
		m_cb_it += n;
		return *this;
	}
	NodeIterator& operator -= ( difference_type n ) {		// it -= n
		m_cb_it -= n;
		return *this;
	}

	NodeIterator operator + ( difference_type n ) const { return NodeIterator<OutputType, Traits, NonConstTraits>( *this ) += n; }
	NodeIterator operator - ( difference_type n ) const { return NodeIterator<OutputType, Traits, NonConstTraits>( *this ) -= n; }

	reference operator [] ( difference_type n ) const { return *( *this + n ); }

	// ***** Comparisons *****
	//
	// This iterator class implements some unusual comparison behavior: two iterators are evaluated by their offset within
	// their respective buffers, even if they point to separate buffers.  When used on synchronized buffers, this allows
	// us to evaluate which of two iterators points to the earlier event.

	template <class OutputType0, class Traits0, class Traits1>
	bool operator == ( const NodeIterator<OutputType0, Traits0, Traits1>& it ) const {
		return index() == it.index();
	}

	template <class OutputType0, class Traits0, class Traits1>
	bool operator != ( const NodeIterator<OutputType0, Traits0, Traits1>& it ) const {
		return index() != it.index();
	}

	template <class OutputType0, class Traits0, class Traits1>
	bool operator < ( const NodeIterator<OutputType0, Traits0, Traits1>& it ) const {
		return index() < it.index();
	}

	template <class OutputType0, class Traits0, class Traits1>
	bool operator > ( const NodeIterator<OutputType0, Traits0, Traits1>& it ) const { return it < *this; }

	template <class OutputType0, class Traits0, class Traits1>
	bool operator <= ( const NodeIterator<OutputType0, Traits0, Traits1>& it ) const { return !( it < *this ); }

	template <class OutputType0, class Traits0, class Traits1>
	bool operator >= ( const NodeIterator<OutputType0, Traits0, Traits1>& it ) const { return !( *this < it ); }

	// ***** Special Methods *****

	// Return the offset within the buffer for this iterator's current location
	// Can be used with at() or operator[], and can be used to compare relative locations
	// of two iterators, even if they don't refer to the same buffer

	size_type index() const {
		return ( size_type ) ( *this - m_buff->begin() + m_buff->firstSampleIndex_ );
	}

	// Return the timestamp associated with the sample this iterator points to

	timestamp_type timestamp() const { return m_buff->timestampAt( index() ); }

	// Tells us whether the index points to a valid place in the buffer.
	// TODO: make sure this is right
	// bool isValid() const { return (index() >= m_buff->beginIndex() && index() <= (m_buff->endIndex() - 1)); }	
};

/*
 * NodeReverseIterator
 *
 * Adapter for reverse iterators on Node classes, preserving some of the
 * special behavior the iterators implement.
 */

template <typename T>
struct NodeReverseIterator : public boost::reverse_iterator<T>
{
	NodeReverseIterator() {}
	explicit NodeReverseIterator( T baseIt ) : boost::reverse_iterator<T>( baseIt ) {}

	typedef typename boost::reverse_iterator<T>::base_type::size_type size_type;

	size_type index() const { return boost::prior( this->base_reference() ).index(); }
	timestamp_type timestamp() const { return boost::prior( this->base_reference() ).timestamp(); }
};

/*
 * NodeInterpolatedIterator
 *
 * Extends the iterator concept to fractional indices, using linear interpolation to return
 * values and timestamps.  This is always a const iterator class.
 */

template<typename OutputType, typename Traits>
struct NodeInterpolatedIterator :
	public boost::iterator<
	std::random_access_iterator_tag,
	typename Traits::value_type,
	typename Traits::difference_type,
	typename Traits::pointer,
	typename Traits::reference>
{
	typedef NodeInterpolatedIterator<OutputType, Traits> self_type;

	typedef boost::iterator<
		std::random_access_iterator_tag,
		typename Traits::value_type,
		typename Traits::difference_type,
		typename Traits::pointer,
		typename Traits::reference> base_iterator;

	typedef typename Traits::size_type size_type;
	typedef typename base_iterator::value_type value_type;
	typedef typename base_iterator::pointer pointer;
	typedef typename base_iterator::reference reference;

	// ***** Member Variables *****

	// Reference to the buffer this iterator indexes
	Node<OutputType>* m_buff;

	// Index location within the buffer
	double m_index;

	// Step size for ++ and similar operators
	double m_step;

	// ***** Constructors *****

	// Default (empty) constructor
	NodeInterpolatedIterator() : m_buff( 0 ), m_index( 0.0 ), m_step( 1.0 ) {}

	// Constructor that should be used primarily by the Node class itself
	NodeInterpolatedIterator( Node<OutputType>* buff, double index, double stepSize )
		: m_buff( buff ), m_index( index ), m_step( stepSize ) {}

	// Copy constructor
	NodeInterpolatedIterator( const self_type& obj ) : m_buff( obj.m_buff ), m_index( obj.m_index ), m_step( obj.m_step ) {}

	// ***** Operators *****
	//
	// Modeled on STL iterators, but using fractional indices.  This class should be considered a sort of random access,
	// bidirectional iterator.  

	NodeInterpolatedIterator& operator = ( const self_type& it ) {
		if( this == &it )
			return *this;
		m_buff = it.m_buff;
		m_index = it.m_index;
		m_step = it.m_step;
		return *this;
	}

	// Dereferencing operators.  Like the non-interpolated version of this iterator, it will calculate the values as needed.
	// This happens within the operator[] method of Node, rather than in this class itself.

	value_type operator * () const { return m_buff->interpolate( m_index ); }
	pointer operator -> () const { return &( operator*() ); }

	// Difference of two iterators (return the difference in indices)
	double operator - ( const self_type& it ) const { return m_index - it.m_index; }

	// Increment and decrement are typically integer operators.  We define their behavior here to change the index by a predetermined
	// step size, set at construction but changeable.

	self_type& operator ++ () {			// ++it
		m_index += m_step;
		return *this;
	}
	self_type operator ++ ( int ) {		// it++
		self_type tmp = *this;
		m_index += m_step;
		return tmp;
	}
	self_type& operator -- () {			// --it
		m_index -= m_step;
		return *this;
	}
	self_type operator -- ( int ) {		// it--
		self_type tmp = *this;
		m_index -= m_step;
		return tmp;
	}

	// These methods change the iterator location by a fractional amount.  Notice that this is NOT scaled by m_step.
	self_type& operator += ( double n ) {		// it += n
		m_index += n;
		return *this;
	}
	self_type& operator -= ( double n ) {		// it -= n
		m_index -= n;
		return *this;
	}

	self_type operator + ( double n ) const { return NodeInterpolatedIterator<OutputType, Traits>( *this ) += n; }
	self_type operator - ( double n ) const { return NodeInterpolatedIterator<OutputType, Traits>( *this ) -= n; }

	reference operator [] ( double n ) const { return *( *this + n ); }


	// ***** Comparisons *****
	//
	// The comparison behavior is the same as for NodeIterator: even if two iterators point to different buffers,
	// they can be compared on the basis of the indices.  Of course, this is only meaningful if the two buffers are synchronized
	// in time.

	template<class OutputType0, class Traits0>
	bool operator == ( const NodeInterpolatedIterator<OutputType0, Traits0>& it ) const { return m_index == it.m_index; }

	template<class OutputType0, class Traits0>
	bool operator != ( const NodeInterpolatedIterator<OutputType0, Traits0>& it ) const { return m_index != it.m_index; }

	template<class OutputType0, class Traits0>
	bool operator < ( const NodeInterpolatedIterator<OutputType0, Traits0>& it ) const { return m_index < it.m_index; }

	template<class OutputType0, class Traits0>
	bool operator > ( const NodeInterpolatedIterator<OutputType0, Traits0>& it ) const { return m_index > it.m_index; }

	template<class OutputType0, class Traits0>
	bool operator <= ( const NodeInterpolatedIterator<OutputType0, Traits0>& it ) const { return !( it < *this ); }

	template<class OutputType0, class Traits0>
	bool operator >= ( const NodeInterpolatedIterator<OutputType0, Traits0>& it ) const { return !( *this < it ); }

	// We can also compare interpolated and non-interpolated iterators.

	template <class OutputType0, class Traits0, class Traits1>
	bool operator == ( const NodeIterator<OutputType0, Traits0, Traits1>& it ) const { return m_index == ( double ) it.index(); }

	template <class OutputType0, class Traits0, class Traits1>
	bool operator != ( const NodeIterator<OutputType0, Traits0, Traits1>& it ) const { return m_index != ( double ) it.index(); }

	template <class OutputType0, class Traits0, class Traits1>
	bool operator < ( const NodeIterator<OutputType0, Traits0, Traits1>& it ) const { return m_index < ( double ) it.index(); }

	template <class OutputType0, class Traits0, class Traits1>
	bool operator > ( const NodeIterator<OutputType0, Traits0, Traits1>& it ) const { return m_index > ( double ) it.index(); }

	template <class OutputType0, class Traits0, class Traits1>
	bool operator <= ( const NodeIterator<OutputType0, Traits0, Traits1>& it ) const { return m_index <= ( double ) it.index(); }

	template <class OutputType0, class Traits0, class Traits1>
	bool operator >= ( const NodeIterator<OutputType0, Traits0, Traits1>& it ) const { return m_index >= ( double ) it.index(); }

	// ***** Special Methods *****

	// Round a fractional index up, down, or to the nearest integer

	self_type& roundDown() {
		m_index = floor( m_index );
		return *this;
	}
	self_type& roundUp() {
		m_index = ceil( m_index );
		return *this;
	}
	self_type& round() {
		m_index = floor( m_index + 0.5 );
		return *this;
	}

	// Increment the iterator by a difference in time rather than a difference in index.  This is less efficient to
	// compute, but can be useful for buffers whose samples are not regularly spaced in time.

	self_type& incrementTime( timestamp_diff_type ts ) {
		if( ts > 0 ) {
			size_type afterIndex = ( size_type ) ceil( m_index );
			if( afterIndex >= m_buff->endIndex() ) {
				m_index = ( double ) m_buff->endIndex();
				return *this;
			}
			timestamp_type target = timestamp() + ts;
			timestamp_type after = m_buff->timestampAt( afterIndex );

			// First of all, find the first integer index with a timestamp greater than our new target time.
			while( after < target ) {
				afterIndex++;
				if( afterIndex >= m_buff->endIndex() ) {
					m_index = ( double ) m_buff->endIndex();
					return *this;
				}
				after = m_buff->timestampAt( afterIndex );
			}

			// Then find the timestamp immediately before that.  We'll interpolate between these two to get
			// the adjusted index.
			timestamp_type before = m_buff->timestampAt( afterIndex - 1 );
			m_index = ( ( target - before ) / ( after - before ) ) + ( double ) ( afterIndex - 1 );
		} else if( ts < 0 ) {
			size_type beforeIndex = ( size_type ) floor( m_index );
			if( beforeIndex < m_buff->beginIndex() ) {
				m_index = ( double ) m_buff->beginIndex() - 1.0;
				return *this;
			}
			timestamp_type target = timestamp() + ts;
			timestamp_type before = m_buff->timestampAt( beforeIndex );

			// Find the first integer index with a timestamp less than our new target time.
			while( before > target ) {
				beforeIndex--;
				if( beforeIndex < m_buff->beginIndex() ) {
					m_index = ( double ) m_buff->beginIndex() - 1.0;
					return *this;
				}
				before = m_buff->timestampAt( beforeIndex );
			}

			// Now find the timestamp immediately after that.  Interpolated to get the adjusted index.
			timestamp_type after = m_buff->timestampAt( beforeIndex + 1 );
			m_index = ( ( target - before ) / ( after - before ) ) + ( double ) beforeIndex;
		}
		// if(ts == 0), do nothing
		return *this;
	}


	// Return (or change) the step size
	double& step() { return m_step; }

	// Return the index within the buffer.  The index is an always-increasing sample number (which means that as the
	// buffer fills, an index will continue to point to the same piece of data until that data is overwritten, at which
	// point that index is no longer valid for accessing the buffer at all.)
	double index() const { return m_index; }

	// Return the timestamp for this particular location in the buffer (using linear interpolation).
	timestamp_type timestamp() const { return m_buff->interpolatedTimestampAt( m_index ); }

	// Tells us whether the index points to a valid place in the buffer.
	bool isValid() const { return ( m_index >= m_buff->beginIndex() && m_index <= ( m_buff->endIndex() - 1 ) ); }
};

/*
 * NodeBase
 *
 * Abstract class that implements common functionality for all Node data types.
 *
 */

class NodeBase : public TriggerSource, public TriggerDestination
{
public:
	typedef uint32_t size_type;

	// ***** Constructors *****

	// Default constructor
	NodeBase() {}

	// Copy constructor: can't copy the mutexes
	NodeBase( NodeBase const& obj ) {}

	NodeBase& operator= ( NodeBase const& obj ) {
		//listeners_ = obj.listeners_;
		return *this;
	}

	// ***** Destructor *****

	virtual ~NodeBase() {
		//clearTriggerSources();
	}

	// ***** Modifiers *****

	virtual void clear() = 0;
	//virtual void insertMissing(timestamp_type timestamp) = 0;	

	// ***** Listener Methods *****
	//
	// We need to keep the buffers of all connected units in sync.  That means any Source or Filter needs to know what its output
	// connects to.  That functionality is accomplished by "listeners": any time a new sample is added to the buffer, the listeners
	// are notified with a corresponding timestamp.  The latter is to ensure that a buffer does not respond to notifications from multiple
	// inputs for the same data point.

	/*void registerListener(NodeBase* listener) {
		if(listener != 0) {
			listenerAccessMutex_.lock();
			listeners_.insert(listener);
			listenerAccessMutex_.unlock();
		}
	}
	void unregisterListener(NodeBase* listener) {
		listenerAccessMutex_.lock();
		listeners_.erase(listener);
		listenerAccessMutex_.unlock();
	}
	void clearListeners() {
		listenerAccessMutex_.lock();
		listeners_.clear();
		listenerAccessMutex_.unlock();
	}
protected:
	// Tell all our listeners about a new data point with the given timestamp
	void notifyListenersOfInsert(timestamp_type timestamp) {
		std::set<NodeBase*>::iterator it;

		listenerAccessMutex_.lock();
		for(it = listeners_.begin(); it != listeners_.end(); it++)
			(*it)->insertMissing(timestamp);
		listenerAccessMutex_.unlock();
	}
	// Tell all our listeners that the buffer has cleared
	void notifyListenersOfClear() {
		std::set<NodeBase*>::iterator it;

		listenerAccessMutex_.lock();
		for(it = listeners_.begin(); it != listeners_.end(); it++)
			(*it)->clear();
		listenerAccessMutex_.unlock();
	}*/

public:
	// ***** Tree-Parsing Methods *****
	//
	// Sometimes we want to find out properties of the initial data source, regardless of what filters it's passed through.  These virtual
	// methods should be implemented differently by Source and Filter classes.

	//virtual SourceBase& source() = 0;		// Return the source at the top of the tree for this unit

	// ***** Mutex Methods *****
	//
	// These methods should be used to acquire a lock whenever a process wants to read values from the buffer.  This would, for example,
	// allow iteration over the contents of the buffer without worrying that the contents will change in the course of the iteration.

	void lock_mutex() { bufferAccessMutex_.enter(); }
	bool try_lock_mutex() { return bufferAccessMutex_.tryEnter(); }
	void unlock_mutex() { bufferAccessMutex_.exit(); }

	// ***** Circular Buffer (STL) Methods *****
	//
	// Certain STL methods (and related queries) that do not depend on the specific data type of the buffer should
	// be available to any object with a NodeBase reference.

	virtual size_type size() = 0;				// Size: how many elements are currently in the buffer
	virtual bool empty() = 0;
	virtual bool full() = 0;
	virtual size_type reserve() = 0;			// Reserve: how many elements are left before the buffer is full
	virtual size_type capacity() const = 0;			// Capacity: how many elements could be in the buffer

	virtual size_type beginIndex() = 0;			// Index of the first sample we still have in the buffer
	virtual size_type endIndex() = 0;			// Index just past the end of the buffer	

	// ***** Timestamp Methods *****
	//
	// Every sample is tagged with a timestamp.  We don't necessarily need to know the type of the sample to retrieve its
	// associated timestamp.  However, we DO need to know the type in order to return an iterator.

	virtual timestamp_type timestampAt( size_type index ) = 0;
	virtual timestamp_type latestTimestamp() = 0;
	virtual timestamp_type earliestTimestamp() = 0;

	virtual size_type indexNearestTo( timestamp_type t ) = 0;
	virtual size_type indexNearestBefore( timestamp_type t ) = 0;
	virtual size_type indexNearestAfter( timestamp_type t ) = 0;

	// ***** Member Variables *****
protected:
	// A collection of the units that are listening for updates on this unit.
	//std::set<NodeBase*> listeners_;

	// This mutex protects access to the underlying buffer.  It is locked every time a sample is written to the buffer,
	// and external systems reading values from the buffer should also acquire at least a shared lock.
	juce::CriticalSection bufferAccessMutex_;

	// This mutex protects the list of listeners.  It prevents a listener from being added or removed while a notification
	// is in progress.
	//boost::mutex listenerAccessMutex_;

	// Keep an internal registry of who we've asked to send us triggers.  It's important to keep
	// a list of these so that when this object is destroyed, all triggers are automatically unregistered.
	//std::set<NodeBase*> triggerSources_;	

	// This list tracks the destinations we are *sending* triggers to (as opposed to the sources we're receiving from above)
	//std::set<NodeBase*> triggerDestinations_;	
};

/*
 * NodeNonInterpolating
 *
 * This class handles all functionality for a Node of a specific data type EXCEPT:
 *   -- Interpolating accessors (for data types that support interpolation, use the more common Node subclass)
 *   -- triggerReceived() which should be implemented by a specific subclass.
 */

template<typename OutputType>
class NodeNonInterpolating : public NodeBase
{
public:
	// Useful type shorthands.  See <boost/circular_buffer.hpp> for details.
	typedef typename boost::container::allocator_traits<std::allocator<OutputType> > Alloc;

	typedef typename boost::circular_buffer<OutputType, Alloc>::value_type value_type;
	typedef typename boost::circular_buffer<OutputType, Alloc>::pointer pointer;
	typedef typename boost::circular_buffer<OutputType, Alloc>::const_pointer const_pointer;
	typedef typename boost::circular_buffer<OutputType, Alloc>::reference reference;
	typedef typename boost::circular_buffer<OutputType, Alloc>::const_reference const_reference;
	typedef typename boost::circular_buffer<OutputType, Alloc>::difference_type difference_type;
	//typedef typename boost::circular_buffer<OutputType,Alloc>::size_type size_type;
	typedef typename boost::circular_buffer<OutputType, Alloc>::capacity_type capacity_type;
	typedef typename boost::circular_buffer<OutputType, Alloc>::array_range array_range;
	typedef typename boost::circular_buffer<OutputType, Alloc>::const_array_range const_array_range;
	typedef typename boost::circular_buffer<OutputType, Alloc>::param_value_type return_value_type;

	// We only support const iterators.  (Modifying data in the buffer is restricted to only a few specialized instances.)

	typedef NodeIterator<OutputType, boost::cb_details::const_traits<Alloc>,
		boost::cb_details::nonconst_traits<Alloc> > const_iterator;
	typedef const_iterator iterator;
	typedef NodeReverseIterator<const_iterator> const_reverse_iterator;
	typedef const_reverse_iterator reverse_iterator;

	template<class O, class T, class NCT> friend struct NodeIterator;

	// ***** Constructors *****

	//Node() : buffer_(0), insertMissingLastTimestamp_(0), numSamples_(0), firstSampleIndex_(0) {}	

	// Recommended constructor: specify the capacity in samples
	explicit NodeNonInterpolating( capacity_type capacity ) : insertMissingLastTimestamp_( 0 ), buffer_( 0 ), numSamples_( 0 ), firstSampleIndex_( 0 ) {
		buffer_ = new boost::circular_buffer<OutputType>( capacity );
		timestamps_ = new boost::circular_buffer<timestamp_type>( capacity );
	}

	// Copy constructor
	NodeNonInterpolating( const NodeNonInterpolating<OutputType>& obj ) : numSamples_( obj.numSamples_ ), firstSampleIndex_( obj.firstSampleIndex_ ) {
		if( obj.buffer_ != nullptr )
			this->buffer_ = new boost::circular_buffer<OutputType>( *obj.buffer_ );
		else
			this->buffer_ = 0;
		if( obj.timestamps_ != nullptr )
			timestamps_ = new boost::circular_buffer<timestamp_type>( *obj.timestamps_ );
		else
			this->timestamps_ = 0;
	}

	// ***** Destructor *****

	virtual ~NodeNonInterpolating() {
		delete buffer_;
		delete timestamps_;
	}

	// ***** Circular Buffer (STL) Methods *****
	//
	// In general we support const methods accessing the contents of the buffer, but only in limited cases can the buffer
	// contents be modified.  Source objects allow inserting objects into the buffer, but Filters require the buffer to contain
	// either the result of the evaluator function or a "missing" value.

	// ***** Accessors *****

	const_iterator begin() { return const_iterator( this, buffer_->begin() ); }
	const_iterator end() { return const_iterator( this, buffer_->end() ); }
	const_reverse_iterator rbegin() { return const_reverse_iterator( end() ); }
	const_reverse_iterator rend() { return const_reverse_iterator( begin() ); }

	const_iterator iteratorAtIndex( size_type index ) { return const_iterator( this, buffer_->begin() ) + ( index - firstSampleIndex_ ); }
	const_reverse_iterator riteratorAtIndex( size_type index ) { return const_reverse_iterator( iteratorAtIndex( index + 1 ) ); }

	return_value_type operator [] ( size_type index ) { return ( *( this->buffer_ ) )[ index - this->firstSampleIndex_ ]; }
	return_value_type at( size_type index ) { return this->buffer_->at( index - this->firstSampleIndex_ ); }
	return_value_type front() { return this->buffer_->front(); }
	return_value_type back() { return this->buffer_->back(); }

	// Two more convenience methods to avoid confusion about what front and back mean!
	return_value_type earliest() { return this->buffer_->front(); }
	return_value_type latest() { return this->buffer_->back(); }

	// In the following methods, check whether the value is missing and calculate it as necessary
	// These methods return a value_type (i.e. not a reference, can't be used to modify the buffer.)
	// However, they internally make use of modifying calls in order to update "missing" values.

	/*return_value_type operator [] (size_type index) {
		return_value_type val = (*buffer_)[index-firstSampleIndex_];
		if(!missing_value<OutputType>::isMissing(val))
			return val;
		return ((*buffer_)[index-firstSampleIndex_] = evaluate(index));
	}
	return_value_type at(size_type index) {
		return_value_type val = buffer_->at(index-firstSampleIndex_);
		if(!missing_value<OutputType>::isMissing(val))
			return val;
		return (buffer_->at(index-firstSampleIndex_) = evaluate(index));
	}
	return_value_type front() {
		return_value_type val = buffer_->front();
		if(!missing_value<OutputType>::isMissing(val))
			return val;
		return (buffer_->front() = evaluate(firstSampleIndex_));
	}
	return_value_type back() {
		return_value_type val = buffer_->back();
		if(!missing_value<OutputType>::isMissing(val) && buffer_->size() > 0)
			return val;
		return (buffer_->back() = evaluate(buffer_->size() - 1 + firstSampleIndex_));
	}*/

	size_type size() { return buffer_->size(); }					// Size: how many elements are currently in the buffer
	bool empty() { return buffer_->empty(); }
	bool full() { return buffer_->full(); }
	size_type reserve() { return buffer_->reserve(); }				// Reserve: how many elements are left before the buffer is full
	size_type capacity() const { return buffer_->capacity(); }		// Capacity: how many elements could be in the buffer

	size_type beginIndex() { return firstSampleIndex_; }					// Index of the first sample we still have in the buffer
	size_type endIndex() { return firstSampleIndex_ + buffer_->size(); }	// Index just past the end of the buffer

	// ***** Modifiers *****

	// Clear all stored samples and timestamps
	void clear() {
		bufferAccessMutex_.enter();
		timestamps_->clear();
		buffer_->clear();
		numSamples_ = firstSampleIndex_ = 0;
		bufferAccessMutex_.exit();

		//notifyListenersOfClear();
	}

	// Insert a new item into the buffer
	void insert( const OutputType& item, timestamp_type timestamp ) {
		this->bufferAccessMutex_.enter();
		if( this->buffer_->full() )
			this->firstSampleIndex_++;
		this->timestamps_->push_back( timestamp );
		this->buffer_->push_back( item );
		this->numSamples_++;
		this->bufferAccessMutex_.exit();

		// Notify anyone who's listening for a trigger
		this->sendTrigger( timestamp );
	}

	// Insert a "missing" item into the buffer.  This is really for the Filter subclasses, but we should provide an implementation
	/*void insertMissing(timestamp_type timestamp) {
		if(timestamp == insertMissingLastTimestamp_)
			return;
		this->bufferAccessMutex_.lock();
		if(this->buffer_->full())
			this->firstSampleIndex_++;
		this->timestamps_->push_back(timestamp);
		this->buffer_->push_back(missing_value<OutputType>::missing());
		this->numSamples_++;
		this->bufferAccessMutex_.unlock();

		this->insertMissingLastTimestamp_ = timestamp;
		this->notifyListenersOfInsert(timestamp);
	}	*/

protected:
	// Subclasses are allowed to change the values stored in their buffers.  Give this a different
	// name to avoid confusion with the behavior of [] and at(), which call evaluate() if the sample
	// is missing.

	reference rawValueAt( size_type index ) { return ( *buffer_ )[ index - firstSampleIndex_ ]; }

public:
	// ***** Timestamp Methods *****
	//
	// Every sample is tagged with a timestamp.  The Filter classes will look up the chain to find the timestamp associated
	// with the Source of any particular sample.  We also support methods to return an iterator to a piece of data most closely
	// matching a given timestamp.

	timestamp_type timestampAt( size_type index ) { return timestamps_->at( index - this->firstSampleIndex_ ); }
	timestamp_type latestTimestamp() { return timestamps_->back(); }
	timestamp_type earliestTimestamp() { return timestamps_->front(); }

	size_type indexNearestBefore( timestamp_type t ) {
		boost::circular_buffer<timestamp_type>::iterator it = std::find_if( timestamps_->begin(), timestamps_->end(), t < boost::lambda::_1 );
		if( it == timestamps_->end() )
			return timestamps_->size() - 1 + this->firstSampleIndex_;
		if( it - timestamps_->begin() == 0 )
			return this->firstSampleIndex_;
		return ( size_type ) ( ( --it ) - timestamps_->begin() ) + this->firstSampleIndex_;
	}
	size_type indexNearestAfter( timestamp_type t ) {
		boost::circular_buffer<timestamp_type>::iterator it = std::find_if( timestamps_->begin(), timestamps_->end(), t < boost::lambda::_1 );
		return std::min<size_type>( ( it - timestamps_->begin() ), timestamps_->size() - 1 ) + this->firstSampleIndex_;
	}
	size_type indexNearestTo( timestamp_type t ) {
		boost::circular_buffer<timestamp_type>::iterator it = std::find_if( timestamps_->begin(), timestamps_->end(), t < boost::lambda::_1 );
		if( it == timestamps_->end() )
			return timestamps_->size() - 1 + this->firstSampleIndex_;
		if( it - timestamps_->begin() == 0 )
			return this->firstSampleIndex_;
		timestamp_diff_type after = *it - t;		// Calculate the distance between the desired timestamp and the before/after values,
		timestamp_diff_type before = t - *( it - 1 );	// then return whichever index gets closer to the target.
		if( after < before )
			return ( size_type ) ( it - timestamps_->begin() ) + this->firstSampleIndex_;
		return ( size_type ) ( ( --it ) - timestamps_->begin() ) + this->firstSampleIndex_;
	}

	const_iterator nearestTo( timestamp_type t ) { return begin() + ( difference_type ) indexNearestTo( t ); }
	const_iterator nearestBefore( timestamp_type t ) { return begin() + ( difference_type ) indexNearestBefore( t ); }
	const_iterator nearestAfter( timestamp_type t ) { return begin() + ( difference_type ) indexNearestAfter( t ); }

	const_reverse_iterator rnearestTo( timestamp_type t ) { return rend() - ( difference_type ) indexNearestTo( t ); }
	const_reverse_iterator rnearestBefore( timestamp_type t ) { return rend() - ( difference_type ) indexNearestBefore( t ); }
	const_reverse_iterator rnearestAfter( timestamp_type t ) { return rend() - ( difference_type ) indexNearestAfter( t ); }

private:
	// Calculate the actual value of one sample.  Behavior of this method will be different for Source and Filter types.
	// virtual OutputType evaluate(size_type index) = 0;

	timestamp_type insertMissingLastTimestamp_;	// The last timestamp that came from insertMissing(), so we can avoid duplication	

protected:
	boost::circular_buffer<OutputType>* buffer_;			// Internal buffer to store or cache values
	boost::circular_buffer<timestamp_type>* timestamps_;	// Internal buffer to hold timestamps for each value

	size_type numSamples_;							// How many samples total we've stored in this buffer
	size_type firstSampleIndex_;					// Index of the first sample that still remains in the buffer
};

/*
 * Node
 *
 * This class handles a node of a specific data type, including the ability to interpolate between samples.  This is the recommended
 * class to use for numeric data types and others for which linear interpolation makes sense.
 */

template<typename OutputType>
class Node : public NodeNonInterpolating<OutputType>
{
public:
	typedef typename std::allocator<OutputType> Alloc;
	typedef NodeInterpolatedIterator<OutputType, boost::cb_details::const_traits<Alloc> > interpolated_iterator;

	typedef typename NodeNonInterpolating<OutputType>::return_value_type return_value_type;
	typedef typename NodeNonInterpolating<OutputType>::capacity_type capacity_type;
	typedef typename NodeNonInterpolating<OutputType>::size_type size_type;

	// ***** Constructors *****
	//
	// Use the same constructors as the non-interpolating version.

	explicit Node( capacity_type capacity ) : NodeNonInterpolating<OutputType>( capacity ) {}
	Node( Node<OutputType> const& obj ) : NodeNonInterpolating<OutputType>( obj ) {}

	// ***** Interpolating Accessors *****
	//
	// These overloaded methods allow querying a location between two samples, using linear
	// interpolation to generate the value.

	return_value_type interpolate( double index ) {
		size_type before = floor( index );				// Find the sample before the interpolated location
		double frac = index - ( double ) before;			// Find the fractional remainder component
		OutputType val1 = this->buffer_->at( before - this->firstSampleIndex_ );
		if( before == this->endIndex() - 1 )
			return val1;
		OutputType val2 = this->buffer_->at( before + 1 - this->firstSampleIndex_ );
		//if(missing_value<OutputType>::isMissing(val1))	// Make sure both values have been calculated
		//	val1 = (buffer_->at(before-firstSampleIndex_) = evaluate(before));
		//if(missing_value<OutputType>::isMissing(val2))
		//	val2 = (buffer_->at(before+1-firstSampleIndex_) = evaluate(before+1));		
		return val1 * ( 1.0 - frac ) + val2 * frac;				// Return the interpolated value
	}

	interpolated_iterator interpolatedBegin( double step = 1.0 ) { return interpolated_iterator( this, ( double ) this->beginIndex(), step ); }
	interpolated_iterator interpolatedEnd( double step = 1.0 ) { return interpolated_iterator( this, ( double ) this->endIndex(), step ); }
	interpolated_iterator interpolatedIteratorAtIndex( double index, double step = 1.0 ) { return interpolated_iterator( this, index, step ); }

	// ***** Interpolating Timestamp Methods *****
	//
	// Using linear interpolation, find the exact relationship between a timestamp and an index in the buffer.
	// Designed to be used in conjunction with the interpolate() method for buffer access.

	// Fractional index --> timestamp
	timestamp_type interpolatedTimestampAt( double index ) {
		size_type before = floor( index );
		double frac = index - ( double ) before;
		timestamp_type ts1 = timestampAt( before );
		if( before == this->endIndex() - 1 )
			return ts1;
		timestamp_type ts2 = timestampAt( before + 1 );
		return ts1 * ( 1.0 - frac ) + ts2 * frac;
	}

	// Timestamp --> fractional index
	double interpolatedIndexForTimestamp( timestamp_type timestamp ) {
		size_type before = this->indexNearestBefore( timestamp );
		if( before >= this->timestamps_->size() - 1 + this->firstSampleIndex_ )		// If it's at the end of the buffer, return the last available timestamp
			return ( double ) before;
		timestamp_type beforeTimestamp = this->timestampAt( before );			// Get the timestamp immediately before
		if( beforeTimestamp >= timestamp )								// If it comes after the requested timestamp, we're at the beginning of the buffer
			return ( double ) before;
		timestamp_type afterTimestamp = this->timestampAt( before + 1 );
		double frac = ( timestamp - beforeTimestamp ) / ( afterTimestamp - beforeTimestamp );
		return ( double ) before + frac;
	}
};
