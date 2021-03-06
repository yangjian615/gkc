/*
 * =====================================================================================
 *
 *       Filename: Timing.h
 *
 *    Description: Handles timing events
 *
 *         Author: Paul P. Hilscher (2010-), 
 *
 *        License: GPLv3+
 * =====================================================================================
 */

#ifndef __GKC_TIMING_H__
#define __GKC_TIMING_H__

#include "Global.h"

#include<ctime>


/**
*  @brief Timing class - governs timing etc.
*
*  @todo Please extend documentation of this class ...
* 
**/
struct Timing {
    
  int    step; ///< Total number of time steps

  double time; ///< Total time
    
  /**
  *    Please Document Me !
  *
  **/
  Timing(int _step = -1, double _time = -1.) : step(_step), time(_time) {};

  /**
  *    Please Document Me !
  *
  **/
  bool operator<=(Timing &b) ;

  /**
  *    Please Document Me !
  *
  **/
  bool operator<=(int step2) ;
  
  /**
  *    Please Document Me !
  *
  **/
  bool operator!=(int step2) ;
  
  /**
  *    Please Document Me !
  *
  **/
  
  bool operator!=(double time2) ;
  /**
  *    Please Document Me !
  *
  **/
    
  bool operator%(Timing &b) ;
  /**
  *    Please Document Me !
  *
  **/
  
  bool check(const Timing &b, const double dt) const;
  /**
  *    Please Document Me !
  *
  **/
  double operator&(Timing h_time) ;
  
  /**
  *    Please Document Me !
  *
  **/
  friend std::ostream& operator<<(std::ostream& output, const Timing& t) 
  {
    output << "Steps : " << t.step << " Time : " << t.time;
    return output;
  };
  
  /**
  *
  * Show time in hour 
  *    Please Document Me !
  *
  *
  **/
  static std::string getRemainingTimeString(Timing timing, Timing maxTiming, time_t start_time) ;
  
  /**
  *    Please Document Me !
  *
  **/
  static std::string TimeStringFromSeconds(int secs) ;

};

#endif // __GKC_TIMING_H__
