#include"BBBsignalQuadrature.h"
#include <iostream>
#include <unistd.h> //for sleep
#include <signal.h>
#include <cstring>
#include<thread>
#include<pthread.h>
// Time/synchronization management
#include <chrono>
// Mathematical functions
#include <cmath> // floor function
// PRU programming
#include <poll.h>
#include <stdio.h>
#include <sys/mman.h>
#include <prussdrv.h>
#include <pruss_intc_mapping.h>
#define PRU_ClockPhys_NUM 1 // PRU1
#define PRU_HandlerSynch_NUM 0 // PRU0
/******************************************************************************
* Local Macro Declarations - Global Space point of View                       *
******************************************************************************/
#define AM33XX_PRUSS_IRAM_SIZE 8192 // Instructions RAM (where .p assembler instructions are loaded)
#define AM33XX_PRUSS_DRAM_SIZE 8192 // Data RAM
#define AM33XX_PRUSS_SHAREDRAM_SIZE 12000 // Data RAM

#define PRU_ADDR        0x4A300000      // Start of PRU memory Page 184 am335x TRM
#define PRU_LEN         0x80000         // Length of PRU memory

#define DDR_BASEADDR 0x80000000 //0x80000000 is where DDR starts, but we leave some offset (0x00001000) to avoid conflicts with other critical data present// Already initiated at this position with LOCAL_DDMinit
#define OFFSET_DDR 0x00001000
#define SHAREDRAM 0x00010000 // Already initiated at this position with LOCAL_DDMinit
#define OFFSET_SHAREDRAM 0x00000000 //Global Memory Map (from the perspective of the host) equivalent with 0x00002000

#define PRU0_DATARAM 0x00000000 //Global Memory Map (from the perspective of the host)// Already initiated at this position with LOCAL_DDMinit
#define PRU1_DATARAM 0x00002000 //Global Memory Map (from the perspective of the host)// Already initiated at this position with LOCAL_DDMinit
#define DATARAMoffset	     0x00000200 // Offset from Base OWN_RAM to avoid collision with some data. // Already initiated at this position with LOCAL_DDMinit

#define PRUSS0_PRU0_DATARAM 0
#define PRUSS0_PRU1_DATARAM 1
#define PRUSS0_PRU0_IRAM 2
#define PRUSS0_PRU1_IRAM 3
#define PRUSS0_SHARED_DATARAM 4

using namespace std;

namespace exploringBBBCKPD {
void* exploringBBBCKPD::CKPD::ddrMem = nullptr; // Define and initialize ddrMem
void* exploringBBBCKPD::CKPD::sharedMem = nullptr; // Define and initialize
void* exploringBBBCKPD::CKPD::pru0dataMem = nullptr; // Define and initialize 
void* exploringBBBCKPD::CKPD::pru1dataMem = nullptr; // Define and initialize
void* exploringBBBCKPD::CKPD::pru_int = nullptr;// Define and initialize
unsigned int* exploringBBBCKPD::CKPD::sharedMem_int = nullptr;// Define and initialize
unsigned int* exploringBBBCKPD::CKPD::pru0dataMem_int = nullptr;// Define and initialize
unsigned int* exploringBBBCKPD::CKPD::pru1dataMem_int = nullptr;// Define and initialize
int exploringBBBCKPD::CKPD::mem_fd = -1;// Define and initialize 

CKPD::CKPD(){// Redeclaration of constructor GPIO when no argument is specified
	// Initialize structure used by prussdrv_pruintc_intc
	// PRUSS_INTC_INITDATA is found in pruss_intc_mapping.h
	tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
	// Allocate and initialize memory
	prussdrv_init();
	// Interrupts
	if (prussdrv_open(PRU_EVTOUT_0) == -1) {// Event PRU-EVTOUT0 - Interrupt from PRU0
	   perror("prussdrv_open(PRU_EVTOUT_0) failed. Execute as root: sudo su or sudo. /boot/uEnv.txt has to be properly configured with iuo. Message: "); 
	  }
	
	if (prussdrv_open(PRU_EVTOUT_1) == -1) {// Event PRU-EVTOUT1 - Interrupt from PRU1
	   perror("prussdrv_open(PRU_EVTOUT_1) failed. Execute as root: sudo su or sudo. /boot/uEnv.txt has to be properly configured with iuo. Message: "); 
	  }
	// Map PRU's interrupts
	// prussdrv.pruintc_init(); // Init handling interrupts from PRUs
	prussdrv_pruintc_init(&pruss_intc_initdata);
	// Clear prior interrupt events
	prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);
	prussdrv_pru_clear_event(PRU_EVTOUT_1, PRU1_ARM_INTERRUPT);
        	
        // Initialize DDM
	LOCAL_DDMinit();
		    // Load and execute the PRU program on the PRU0
	    pru0dataMem_int[0]=1;// set the number of clocks that defines the Quarter period of the clock.
	if (prussdrv_exec_program(PRU_HandlerSynch_NUM, "./PRUassLecture.bin") == -1){
		perror("prussdrv_exec_program non successfull writing of PRUassLecture.bin");
	}
	
	
	    // Load and execute the PRU program on the PRU1
	    //pru1dataMem_int[0]=static_cast<unsigned int>(this->NumClocksQuarterPeriodPRUclock+this->AdjCountsFreq); // set the number of clocks that defines the Quarter period of the clock. 
	if (prussdrv_exec_program(PRU_ClockPhys_NUM, "./PRUassTrigSigScriptHist1Sig.bin") == -1){
		perror("prussdrv_exec_program non successfull writing of PRUassTrigSigScriptHist1Sig.bin");
	}
	//prussdrv_pru_enable(PRU_ClockPhys_NUM);
	sleep(20);
	
	// INICIALIZACION PRU1 SEÑALES
	pru1dataMem_int[0]=static_cast<unsigned int>(1); // pOSICION DE MEMORIA 0 HE PUESTO UN 1 set command. Generate signals. Takes around 900000 clock ticks
	prussdrv_pru_send_event(22);//INTERRUPTCION PRU1

	/*retInterruptsPRU1=prussdrv_pru_wait_event_timeout(PRU_EVTOUT_1,WaitTimeInterruptPRU1);

	if (retInterruptsPRU1>0){
	prussdrv_pru_clear_event(PRU_EVTOUT_1, PRU1_ARM_INTERRUPT);// So it has time to clear the interrupt for the later iterations
	}
	else if (retInterruptsPRU1==0){
		prussdrv_pru_clear_event(PRU_EVTOUT_1, PRU1_ARM_INTERRUPT);// So it has time to clear the interrupt for the later iterations
		cout << "CKPD::HandleInterruptSynchPRU took to much time for the ClockHandler. Reset PRU1 if necessary." << endl;	
	}
	else{
		prussdrv_pru_clear_event(PRU_EVTOUT_1, PRU1_ARM_INTERRUPT);// So it has time to clear the interrupt for the later iterations
		cout << "PRU1 interrupt poll error" << endl;
	}*/
	
	//FIN PRU1 SEÑALES
	this->TimePointClockCurrentInitial=ClockWatch::now();
	// Absolute time reference	
	//std::chrono::nanoseconds duration_back(static_cast<unsigned long long int>(floor(static_cast<long double>(std::chrono::duration_cast<std::chrono::nanoseconds>(TimePointClockCurrentInitial.time_since_epoch()).count())/static_cast<long double>(this->TimeAdjPeriod))*static_cast<long double>(this->TimeAdjPeriod)));
	//this->TimePointClockCurrentInitial=ClockWatch::time_point(duration_back);
	this->requestWhileWait = this->SetWhileWait();// Used with non-busy wait
	//this->TimePointClockCurrentInitialMeas=this->TimePointClockCurrentFinal-std::chrono::nanoseconds(this->duration_FinalInitialDriftAuxArrayAvg);// Important that it is discounted the time to enter the interrupt so that in this way the signal generation in the PRU is reference to an absolute time.
	cout << "Generating clock output..." << endl;
}
int CKPD::HandleInterruptSynchPRU(){ // Uses output pins to clock subsystems physically generating qubits or entangled qubits
clock_nanosleep(CLOCK_REALTIME,TIMER_ABSTIME,&requestWhileWait,NULL);//CLOCK_TAI,CLOCK_REALTIME// https://opensource.com/article/17/6/timekeeping-linux-vms
// Important, the following line at the very beggining to reduce the command jitter
	prussdrv_pru_send_event(21);//INTERRUPCION PARA EL PRU0
	//this->TimePointClockTagPRUfinal=Clock::now();// Compensate for delays
	// Set regular priority
	//this->setMaxRrPriority(PriorityValRegular);
	//retInterruptsPRU0=prussdrv_pru_wait_event_timeout(PRU_EVTOUT_0,WaitTimeInterruptPRU0);// First interrupt sent to measure time
	//  PRU long execution making sure that notification interrupts do not overlap
	retInterruptsPRU0=prussdrv_pru_wait_event_timeout(PRU_EVTOUT_0,WaitTimeInterruptPRU0);

	/*
	//cout << "dPRUoffsetDriftErrorAvg: " << dPRUoffsetDriftErrorAvg << endl;
	//cout << "AccumulatedErrorDrift: " << AccumulatedErrorDrift << endl;
	cout << "AccumulatedErrorDriftAux: " << AccumulatedErrorDriftAux << endl;
	cout << "PRUoffsetDriftErrorAvg: " << PRUoffsetDriftErrorAvg << endl;
	cout << "PRUoffsetDriftErrorAbsAvg: " << PRUoffsetDriftErrorAbsAvg << endl;
	cout << "PRUoffsetDriftErrorAbsAvgAux: " << PRUoffsetDriftErrorAbsAvgAux << endl;
	cout << "SynchTrigPeriod: " << SynchTrigPeriod << endl;
	cout << "InstantCorr: " << InstantCorr << endl;
	////cout << "RecurrentAuxTime: " << RecurrentAuxTime << endl;
	//cout << "pru0dataMem_int3aux: " << pru0dataMem_int3aux << endl;
	////cout << "SynchRem: " << SynchRem << endl;
	cout << "this->AdjPulseSynchCoeffAverage: " << this->AdjPulseSynchCoeffAverage << endl;
	cout << "this->duration_FinalInitialMeasTrigAuxAvg: " << this->duration_FinalInitialMeasTrigAuxAvg << endl;
	*/

	//cout << "retInterruptsPRU0: " << retInterruptsPRU0 << endl;
	if (retInterruptsPRU0>0){
		prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);// So it has time to clear the interrupt for the later iterations
	}
	else if (retInterruptsPRU0==0){
		prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);// So it has time to clear the interrupt for the later iterations
		cout << "GPIO::ReadTimeStamps took to much time for the TimeTagg. Timetags might be inaccurate. Reset PRUO if necessary." << endl;		
	}
	else{
		prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);// So it has time to clear the interrupt for the later iterations
		cout << "PRU0 interrupt poll error" << endl;
	}


this->requestWhileWait = this->SetWhileWait();// Used with non-busy wait
this->TimePointClockCurrentInitialMeas=this->TimePointClockCurrentFinal;
// Compute error
/*if (retInterruptsPRU1>0){
	// Compute absolute error
	if (this->CounterHandleInterruptSynchPRU>=WaitCyclesBeforeAveraging){// Error should not be filtered
	this->TimePointClockCurrentAdjError=0.0;
	}
	else{
		this->TimePointClockCurrentAdjError=0.0;
	}

	// Error filtering
	// Limit applied error correction
	if (this->TimePointClockCurrentAdjFilError>this->MaxTimePointClockCurrentAdjFilError){this->TimePointClockCurrentAdjFilError=this->MaxTimePointClockCurrentAdjFilError;}
	else if (this->TimePointClockCurrentAdjFilError<this->MinTimePointClockCurrentAdjFilError){this->TimePointClockCurrentAdjFilError=this->MinTimePointClockCurrentAdjFilError;}
	
	// PID implementation
	//this->PIDcontrolerTime();
	
	// Apply period scaling selected by the user
	if (this->CounterHandleInterruptSynchPRU<WaitCyclesBeforeAveraging){// Do not apply the averaging in the first ones since everything is adjusting
		this->AdjCountsFreq=0.0;
	}
	else{
		this->AdjCountsFreq=this->AdjCountsFreqHolder/PRUclockStepPeriodNanoseconds/4.0;// divided by 4 because it is for Quarter period for the PRU1.
	}
	this->MinAdjCountsFreq=-this->NumClocksQuarterPeriodPRUclock+static_cast<double>(MinNumPeriodColcksPRUnoHalt);
	if (this->AdjCountsFreq>this->MaxAdjCountsFreq){this->AdjCountsFreq=this->MaxAdjCountsFreq;}
	else if(this->AdjCountsFreq<this->MinAdjCountsFreq){this->AdjCountsFreq=this->MinAdjCountsFreq;}
}


PRU1QuarterClocksAux=static_cast<unsigned int>(this->NumClocksQuarterPeriodPRUclock+this->AdjCountsFreq+this->TimePointClockCurrentAdjFilErrorApplied/PRUclockStepPeriodNanoseconds/4.0);// Here the correction is inserted

if (PRU1QuarterClocksAux>this->MaxNumPeriodColcksPRUnoHalt){PRU1QuarterClocksAux=this->MaxNumPeriodColcksPRUnoHalt;}
else if (PRU1QuarterClocksAux<this->MinNumPeriodColcksPRUnoHalt){PRU1QuarterClocksAux=this->MinNumPeriodColcksPRUnoHalt;}

this->CounterHandleInterruptSynchPRU++;// Update counter

if (this->CounterHandleInterruptSynchPRU%30==0){
	this->CounterHandleInterruptSynchPRU=0;// Reset value
	if (PlotPIDHAndlerInfo){	
	cout << "this->duration_FinalInitialDriftAuxArrayAvg: " << this->duration_FinalInitialDriftAuxArrayAvg << " ns." << endl;
	cout << "this->TimePointClockCurrentAdjError: " << this->TimePointClockCurrentAdjError << endl;
	cout << "this->TimePointClockCurrentAdjFilError: " << this->TimePointClockCurrentAdjFilError << endl;
	cout << "this->TimePointClockCurrentAdjFilErrorApplied: " << this->TimePointClockCurrentAdjFilErrorApplied << endl;
	//cout << "this->TimePointClockCurrentAdjFilErrorAppliedOld: " << this->TimePointClockCurrentAdjFilErrorAppliedOld << endl;
	//cout << "PRU1QuarterClocksAux: " << PRU1QuarterClocksAux << endl;
	}
}*/

// ApplY PRU command for next roung
pru0dataMem_int[0]=1;//Information sent to and grabbed by PRU0

return 0;// All ok
}

/*int CKPD::PIDcontrolerTime(){
TimePointClockCurrentAdjFilErrorDerivative=0.0;
TimePointClockCurrentAdjFilErrorIntegral=0.0;

this->TimePointClockCurrentAdjFilErrorApplied=PIDconstant*TimePointClockCurrentAdjFilError+PIDintegral*TimePointClockCurrentAdjFilErrorIntegral+PIDderiv*TimePointClockCurrentAdjFilErrorDerivative;
TimePointClockCurrentAdjFilErrorLast=TimePointClockCurrentAdjFilError;// Update
CounterHandleInterruptSynchPRUlast=CounterHandleInterruptSynchPRU;// Update
return 0; // All ok
}*/

struct timespec CKPD::SetWhileWait(){
	struct timespec requestWhileWaitAux;
	// Relative incrment
	this->TimePointClockCurrentFinal=this->TimePointClockCurrentInitial+std::chrono::nanoseconds(this->TimeAdjPeriod);
	this->TimePointClockCurrentInitial=this->TimePointClockCurrentFinal;
		
	auto duration_since_epochFutureTimePoint=this->TimePointClockCurrentFinal.time_since_epoch();
	// Convert duration to desired time
	long long int TimePointClockCurrentFinal_time_as_count = static_cast<long long int>(std::chrono::duration_cast<std::chrono::nanoseconds>(duration_since_epochFutureTimePoint).count());//-static_cast<long long int>(this->TimeClockMarging); // Add an offset, since the final barrier is implemented with a busy wait 
	//cout << "TimePointClockCurrentFinal_time_as_count: " << TimePointClockCurrentFinal_time_as_count << endl;

	requestWhileWaitAux.tv_sec=(int)(TimePointClockCurrentFinal_time_as_count/((long)1000000000));
	requestWhileWaitAux.tv_nsec=(long)(TimePointClockCurrentFinal_time_as_count%(long)1000000000);
	return requestWhileWaitAux;
}

/*int CKPD::SetFutureTimePoint(){
	this->TimePointClockCurrentFinal=this->TimePointClockCurrentInitial+std::chrono::nanoseconds(this->TimeAdjPeriod);//-std::chrono::nanoseconds(duration_FinalInitialDriftAux);//-std::chrono::nanoseconds(static_cast<unsigned long long int>(this->PIDconstant*static_cast<double>(this->TimePointClockCurrentAdjFilError)));
	this->TimePointClockCurrentInitial=this->TimePointClockCurrentFinal;
	return 0; // All Ok
}*/

/*****************************************************************************
* Local Function Definitions                                                 *
*****************************************************************************/

int CKPD::LOCAL_DDMinit(){
    //void *DDR_regaddr1, *DDR_regaddr2, *DDR_regaddr3;    
    prussdrv_map_prumem(PRUSS0_SHARED_DATARAM, &sharedMem);// Maps the PRU shared RAM memory is then accessed by an array.
    sharedMem_int = (unsigned int*) sharedMem;
        
    prussdrv_map_prumem(PRUSS0_PRU0_DATARAM, &pru0dataMem);// Maps the PRU0 DRAM memory to input pointer. Memory is then accessed by an array.
    pru0dataMem_int = (unsigned int*) pru0dataMem;
    
    prussdrv_map_prumem(PRUSS0_PRU1_DATARAM, &pru1dataMem);// Maps the PRU1 DRAM memory to input pointer. Memory is then accessed by an array.
    pru1dataMem_int = (unsigned int*) pru1dataMem;

    return 0;
}

/*int CKPD::RelativeNanoSleepWait(unsigned int TimeNanoSecondsSleep){
struct timespec ts;
ts.tv_sec=(int)(TimeNanoSecondsSleep/((long)1000000000));
ts.tv_nsec=(long)(TimeNanoSecondsSleep%(long)1000000000);
clock_nanosleep(CLOCK_REALTIME, 0, &ts, NULL); //

return 0; // All ok
}*/

int CKPD::Killsignalpru{
	if (prussdrv_exec_program(PRU_HandlerSynch_NUM, "./PRUkillSignal.bin") == -1){
		perror("prussdrv_exec_program non successfull writing of PRUkillSignal.bin");
	}
return 0;
}

int CKPD::DisablePRUs(){
// Disable PRU and close memory mappings
prussdrv_pru_disable(PRU_ClockPhys_NUM);
prussdrv_pru_disable(PRU_HandlerSynch_NUM);

return 0;
}
CKPD::~CKPD() {
	this->Killsignalpru();
//	this->unexportGPIO();
	this->DisablePRUs();
	//fclose(outfile); 
	prussdrv_exit();
	//munmap(ddrMem, 0x0FFFFFFF);
	//close(mem_fd); // Device
	//close(tfd);// close the time descriptor
}

}

using namespace exploringBBBCKPD;

int main(int argc, char const * argv[]){

 cout << "CKPDagent started..." << endl;
 
 CKPD CKPDagent; // Initiate the instance
 
 CKPDagent.m_start(); // Initiate in start state.
 
 bool isValidWhileLoop=true;
 if (CKPDagent.getState()==CKPD::APPLICATION_EXIT){isValidWhileLoop = false;}
 else{isValidWhileLoop = true;}
 
 //CKPDagent.GenerateSynchClockPRU();// Launch the generation of the clock
 cout << "Starting to actively adjust clock output..." << endl;
 
 while(isValidWhileLoop){ 
 	//CKPDagent.acquire();
   //try{
 	//try {
    	// Code that might throw an exception 
 	// Check if there are need messages or actions to be done by the node
 	
       switch(CKPDagent.getState()) {
           case CKPD::APPLICATION_RUNNING: {               
               // Do Some Work
               //CKPDagent.HandleInterruptSynchPRU();
               //prussdrv_pru_send_event(22);
               break;
           }

           default: {
               // ErrorHandling Throw An Exception Etc.
           }

        } // switch
        
    } // while
  cout << "Exiting the BBBclockKernelPhysicalDaemon" << endl;
  
  
 return 0; // Everything Ok
}
