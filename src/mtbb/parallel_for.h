/* 
 * parallel_for implementation on top of MassiveThreads


 it supports an interface similar to TBB's parallel_for
 (http://software.intel.com/en-us/node/467906)

It supports the following interfaces

// for(i=first; i<last; i++) f(i) 
template<typename Index, typename Func>
Func parallel_for( Index first, Index_type last, const Func& f );

// for(i=first; i<last; i += step) f(i) 
template<typename Index, typename Func>
Func parallel_for( Index first, Index_type last, 
                   Index step, const Func& f );

// divide range into subranges and call body(range) for small ranges 
template<typename Range, typename Body> 
void parallel_for( const Range& range, const Body& body );

The main difference from TBB is they do not take
partitioner and task_group_context parameters.

range is any object implementing "Range concept"
(http://software.intel.com/en-us/node/467886).  Among others it has
is_divisible() method, empty() method, and a constructor to split it
into two sub-ranges.  

TBB programs usually use one of block_range, block_range2d, or
block_range3d classes to make such range objects.  You can simply use
TBB's corresponding classes (we can roll our own, but the implemention
would be identical).  

body is any object that takes a range.

To use this file on top of MassiveThreads, you should
include the following files.

#include <myth.h>
#include <mtbb/parallel_for.h>

// includes below are optional. 
// include what you need.
#include <tbb/block_range.h>
#include <tbb/block_range2d.h>
#include <tbb/block_range3d.h>

 */

#pragma once
#include <mtbb/task_group.h>

namespace mtbb {

  template<typename Range, typename Body> 
    void parallel_for( const Range& range, Body& body );

  template<typename Range, typename Body>
    struct parallel_for_callable {
      const Range & range;
      Body & body;
    parallel_for_callable(const Range & range_, Body & body_) :
      range(range_), body(body_) {}
      void operator() () {
        mtbb::parallel_for(range, body);
      }
    };
  
  template<typename Range, typename Body> 
    void parallel_for( const Range& range, Body& body ) {
    if (range.empty()) {
      return;
    } else if (!range.is_divisible()) {
      body(range);
    } else {
      task_group tg;
      Range left(range);
      const Range right(left, tbb::split());
      //tg.run(parallel_for_callable<Range,Body>(left, body));
      tg.run_(parallel_for_callable<Range,Body>(left, body), __FILE__, __LINE__);
      mtbb::parallel_for(right, body);
      //tg.wait();
      tg.wait_(__FILE__, __LINE__);
    }
  }
  
  template<typename Index, typename Func>
    struct parallel_for_aux_callable {
      Index first;
      Index a;
      Index b;
      Index step;
      Index grainsize;
      const Func & f;
    parallel_for_aux_callable(Index first_, Index a_, Index b_, Index step_, Index grainsize_, const Func & f_) :
      first(first_), a(a_), b(b_), step(step_), grainsize(grainsize_), f(f_) {}
      void operator() () {
	parallel_for_aux(first, a, b, step, grainsize, f);
      }
    };
  
  template<typename Index, typename Func>
    Func parallel_for(Index first, Index last, Index step,
		      const Func& f);
  
  template<typename Index, typename Func>
    Func parallel_for(Index first, Index last, 
		      const Func& f);

  template<typename Index, typename Func>
    Func parallel_for_aux(Index first, 
			  Index a, Index b, Index step, Index grainsize,
			  const Func& f) {
    if (b - a <= grainsize) {
      if (grainsize == 1)
        f(first + a * step);
      else
        f(first + a * step, first + b * step);
    } else {
      mtbb::task_group tg;
      const Index c = a + (b - a) / 2;
      //tg.run(parallel_for_aux_callable<Index,Func>(first, a, c, step, grainsize, f));
      tg.run_(parallel_for_aux_callable<Index,Func>(first, a, c, step, grainsize, f), __FILE__, __LINE__);
      parallel_for_aux(first, c, b, step, grainsize, f);
      //tg.wait();
      tg.wait_(__FILE__, __LINE__);
    }
    return f;
  }
  
  template<typename Index, typename Func>
    Func parallel_for(Index first, Index last, Index step, Index grainsize,
		      const Func& f) {
    return parallel_for_aux(first, 0, 1 + (last - first - 1) / step, step, grainsize, f);
  }
  
  template<typename Index, typename Func>
    Func parallel_for(Index first, Index last, Index step,
		      const Func& f) {
    //return parallel_for_aux(first, 0, (last - first) / step, step, 1, f);
    return parallel_for_aux(first, 0, (last - first + step - 1) / step, step, 1, f);
  }
  
  template<typename Index, typename Func>
    Func parallel_for(Index first, Index last, 
		      const Func& f) {
    return parallel_for_aux(first, 0, (last - first), 1, 1, f);
  }
 } /* namespace mtbb */


