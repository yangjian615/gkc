/*
 * =====================================================================================
 *
 *       Filename: Control.h
 *
 *    Description: Handles signals and other triggers
 *
 *         Author: Paul P. Hilscher (2010-), 
 *
 *        License: GPLv3+
 * =====================================================================================
 */

#ifndef __CONTROL_H_
#define __CONTROL_H_

#include "Global.h"
#include "Setup.h"
#include "Parallel/Parallel.h"
#include "Analysis/Diagnostics.h"


extern int control_triggered_signal;

/**
*    @brief Controls limits and signals
*
*    Main reason is the catching of Unix SIGNALS, such 
*    as SIGTERM and SIGFPE. Setting a flag, the main loop
*    in GKC is left and the program is shut down nicely.
*
*    The other purpose is to control various triggers, such
*    as large electric field energy, which indicates an
*    errorneous simulations. Also in this case the simulation
*    is stopped.
*  
*    Conditions
*     Signals received SIGTERM, SIGTRAP, SIGFPE
*     Energy exceeds limit
*     Time limit reached
*     File trigger
*
*     ## Setup ##
*
*        if (setup->get("Control.useControlFile", 0)) {
*        maxKineticEnergy  = setup->get("Control.MaxKineticEnergy", 1.e355);
*        maxElectricEnergy = setup->get("Control.MaxElectricEnergy", 1.e355);
*        maxMagneticEnergy = setup->get("Control.MaxMagneticEnergy", 1.e355);
*
*
**/
class Control : public IfaceGKC {
   

  bool useControlFile;

  class CntrlID {
    
    bool ok;                   ///< True if condition is met, False otherwise
    std::string error_message; ///< Error message describing reason of failure
   public:
       
    CntrlID() { ok=true; error_message="";};

    bool check(bool isOK, std::string message) {

      // Append messages of failure
      if(isOK == false) {
        ok = false;
        error_message += std::string("\n") + message;
      }
      return isOK && ok;
    };
       
    bool isOK() { return ok; };
    std::string getMessage() { return error_message;};
    
  };

  Parallel    *parallel;
  Diagnostics *diagnostics;


  CntrlID  cntrl;
    
  double maxKineticEnergy ; ///< set with ("Control.MaxKineticEnergy" , double), default : 1.e355
  double maxElectricEnergy; ///< set with ("Control.MaxElectricEnergy", double), default : 1.e355
  double maxMagneticEnergy; ///< set with ("Control.MaxMagneticEnergy", double), default : 1.e355

  /**
  *   
  *   @brief Sets the Unix signal handler for
  *
  *   SIGINT, SIGFPE, SIGUSR1, SIGUSR2
  *
  *   @todo check for other signals to capture
  *
  **/
  void setSignalHandler();

  time_t startTime;        ///< Starting time of simulation 
  int maxRunningTime;      ///< maximum running time of program ("Control.MaxRunningTime", TimeString), default : "0s"
     
 public:
  
  /**
  *    @brief constructors
  *
  *    Setup variables
  *
  *    @xrefitem setup "Todo1" "Todo List1" 
  *    @xrefitem setup "Todo2" "Todo List2" 
  *    @xrefitem setup "Todo3" "Todo List3" 
  *
  *
  **/
  Control(Setup *setup, Parallel *_parallel, Diagnostics *_diagnotiscs);

  /**
  *    @brief constructors
  *
  *    Setup variables
  *
  *
  **/
 ~Control();

  /**
  *   
  *   @brief name of control file name
  *
  *   The code regularly (currently each time step) checks for
  *   the existence of a stop file. If it's exists, the program
  *   triggers Cntrl.OK = False, and the main loop is exit.
  *
  *   @note The file name is determined by gkc_ + job_number + ".stop" 
  *
  **/
  std::string cntrl_file_name;

  /**
  *    @brief please document me
  *
  *
  **/ 
  void printLoopStopReason();

  /**
  *    @brief please document me
  *
  *
  **/ 
  void runningException(int status, char *error_message=NULL);

  /**
  *    @brief please document me
  *
  *
  **/ 
  bool checkOK(Timing timing, Timing maxTiming);
    
  /**
  *    @brief please document me
  *
  *
  **/ 
  void signalForceExit(bool val);

  /**
  *    @brief please document me
  *
  *
  **/ 
  void printOn(std::ostream &output) const;

};


#endif // __CONTROL_H_
