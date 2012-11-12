/*
 * =====================================================================================
 *
 *       Filename: GKC.cpp
 *
 *    Description: Main file
 *
 *         Author: Paul P. Hilscher (2010-), 
 *
 *        License: GPLv3+
 * =====================================================================================
 */

#include "config.h"

#include "GKC.h"
#include "Setup.h"
#include "Plasma.h"

#include "FieldsFFT.h"
#include "FieldsHermite.h"

#include "FFTSolver/FFTSolver_fftw3.h"

#include "TimeIntegration.h"
#include "TimeIntegration_PETSc.h"

#include "Vlasov/Vlasov_Cilk.h"
#include "Vlasov/Vlasov_Aux.h"
#include "Vlasov/Vlasov_Optim.h"
#include "Vlasov/Vlasov_Island.h"

#include "Geometry/GeometrySA.h"
#include "Geometry/Geometry2D.h"
#include "Geometry/GeometryShear.h"
#include "Geometry/GeometrySlab.h"
#include "Geometry/GeometryCHEASE.h"

#include "System.h"

#include "Eigenvalue/Eigenvalue_SLEPc.h"
#include "Visualization/Visualization_Data.h"



// TODO : Linux uses UTF-8 encoding for chars per default, Windows not, how to deal with unicode ?

GKC::GKC(Setup *_setup) : setup(_setup)  
{

    // Read Setup 
    std::string fft_solver_name = setup->get("GKC.FFTSolver", "fftw3");
    std::string psolver_type    = setup->get("Fields.Solver", "DFT");
    std::string vlasov_type     = setup->get("Vlasov.Solver", "Aux");
    std::string collision_type  = setup->get("Collisions.Solver", "None");
    gkc_SolType                 = setup->get("GKC.Type", "IVP");
    std::string geometry_Type   = setup->get("GKC.Geometry", "Geometry2D");


   /////////////////// Load subsystems ////////////////
    parallel  = new Parallel(setup);
    bench     = new Benchmark(setup, parallel); 

    parallel->print("Initializing GKC++\n");

    fileIO    = new FileIO(parallel, setup);
    grid      = new Grid(setup, parallel, fileIO);
    
    if     (geometry_Type == "GeometrySA") geometry  = new GeometrySA(setup, grid, fileIO);
    else if(geometry_Type == "Geometry2D") geometry  = new Geometry2D(setup, grid, fileIO);
    else if(geometry_Type == "Slab"      ) geometry  = new Geometry2D(setup, grid, fileIO);
    else check(-1, DMESG("No such Geometry"));

    plasma    = new Plasma(setup, fileIO, geometry);


    
    if(fft_solver_name == "") check(-1, DMESG("No FFT Solver Name given"));
#ifdef FFTW2MPI
//    else if(fft_solver_name == "fftw2_mpi")  fftsolver = new FFTSolver_fftw2_mpi    (setup, parallel, geometry);
#endif
#ifdef FFTW3
   else if(fft_solver_name == "fftw3") fftsolver = new FFTSolver_fftw3    (setup, parallel, geometry);
#endif
   else check(-1, DMESG("No such FFTSolver name"));
 

    // Load Field solver
    if     (psolver_type == "DFT"  ) fields   = new FieldsFFT(setup, grid, parallel, fileIO, geometry, fftsolver);
#ifdef GKC_HYPRE
    else if(psolver_type == "Hypre"  ) fields   = new FieldsHypre(setup, grid, parallel, fileIO,geometry, fftsolver);
#endif
    else if(psolver_type == "Hermite") fields   = new FieldsHermite(setup, grid, parallel, fileIO,geometry);
    else    check(-1, DMESG("No such Fields Solver"));

    // Load Collisonal Operator
    if     (collision_type == "None" ) collisions = new Collisions                (grid, parallel, setup, fileIO, geometry); 
    else if(collision_type == "LB"   ) collisions = new Collisions_LenardBernstein(grid, parallel, setup, fileIO, geometry); 
    else    check(-1, DMESG("No such Collisions Solver"));
    
    // Load Vlasov Solver
    if(vlasov_type == "None" ) check(-1, DMESG("No Vlasov Solver Selected"));
    else if(vlasov_type == "Cilk"   ) vlasov  = new VlasovCilk  (grid, parallel, setup, fileIO, geometry, fftsolver, bench, collisions);
    else if(vlasov_type == "Aux"    ) vlasov  = new VlasovAux   (grid, parallel, setup, fileIO, geometry, fftsolver, bench, collisions);
    else if(vlasov_type == "Island" ) vlasov  = new VlasovIsland(grid, parallel, setup, fileIO, geometry, fftsolver, bench, collisions);
    else if(vlasov_type == "Optim"  ) vlasov  = new VlasovOptim (grid, parallel, setup, fileIO, geometry, fftsolver, bench, collisions);
    else   check(-1, DMESG("No such Fields Solver"));


    analysis = new Analysis(parallel, vlasov, fields, grid, setup, fftsolver, fileIO, geometry); 
    visual   = new Visualization_Data(grid, parallel, setup, fileIO, vlasov, fields);
    event    = new Event(setup, grid, parallel, fileIO, geometry);
    
    eigenvalue = new Eigenvalue_SLEPc(fileIO, setup, grid, parallel); 

    
    /////////////////////////////////////////////////////////////////////////////

    // call before time integration (due to eigenvalue solver)
    init           = new  Init(parallel, grid, setup, fileIO, vlasov, fields, geometry);
    
    control   = new Control(setup, parallel, analysis);
    particles = new TestParticles(fileIO, setup, parallel);
   

    timeIntegration = new TimeIntegration(setup, grid, parallel, vlasov, fields, particles, eigenvalue, bench);
  
    // Optimize values to speed up computation
    bench->bench(vlasov, fields);
    
    printSettings();   
    setup->check_config();

}
          

int GKC::mainLoop()   
{

   parallel->print("Running main loop");

   if (gkc_SolType == "IVP") {
 
            Timing timing(0,0.) ;
            // this is ugly here ...
            timeIntegration->setMaxLinearTimeStep(eigenvalue, vlasov, fields);

            bench->start("MainLoop");
            for(; control->checkOK(timing, timeIntegration->maxTiming);){
       
              // integrate for one time-step, give current dt as output
               double dt = timeIntegration->solveTimeStep(vlasov, fields, particles, timing);       

               // Analysis results and output data if necessary
               // check if performed in writeData
               vlasov->writeData(timing, dt);
               fields->writeData(timing, dt);
               visual->writeData(timing, dt);
               analysis->writeData(timing, dt);
               
               event->checkEvent(timing, vlasov, fields);
        
               //if(timing.step %  5000 == 0 && parallel->myRank == 0) std::cout << System::getProcessStatusString();

               // Make here scalar operations
           }
           bench->stop("MainLoop");

   
        control->printLoopStopReason();

   }  
   else if(gkc_SolType == "Eigenvalue") {
   eigenvalue->solve(vlasov, fields, visual, control);

   } else  check(-1, DMESG("No Such gkc.Type Solver"));

   parallel->print("Simulation finished normally ... ");

   return 0;
}


GKC::~GKC()
{
   // Shutdown submodules : ORDER IS IMPORTANT !!
        delete fields;
        delete visual;
        delete vlasov;
        delete geometry;
        delete grid;
        delete analysis;
        delete particles;
        delete eigenvalue;
        delete fileIO;
        // no need to delete table ?! Anyway SLEPc/PETSc seems to crash
        // some times FFT crashes, be sude that it is below fileIO (hdf-5 is closed)
        delete fftsolver;
        delete control;
        delete parallel;
        delete init;
        delete timeIntegration;
        delete bench;
}



void GKC::printSettings() 
{
    std::stringstream infoStream;
  
    time_t start_time = std::time(0); 
  infoStream 
            << "Welcome from " << PACKAGE_NAME << " (" << PACKAGE_VERSION <<")  " << PACKAGE_BUGREPORT <<  "      Date :  " << std::ctime(&start_time)         
            << "-------------------------------------------------------------------------------" << std::endl
            << *grid << *plasma << *fileIO << *setup << *vlasov  << *fields << *geometry << *init << *parallel << *fftsolver << *timeIntegration;
            infoStream << *control << *bench << std::endl;
            infoStream << "-------------------------------------------------------------------------------" << std::endl << std::endl << std::flush;

   parallel->print(infoStream.str());

}
    
