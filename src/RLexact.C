/* Main program file RLexact.C 
* Holding the main loop, definitions, and memory allocation/deallocation 
* Last change: SJ 19.12.16
*
============================================
*
* RLexact: The exact diagonalization package
* Christian Rischel & Kim Lefmann, 26.02.94  
* Version 4.0, September 2017
* 
============================================
*/

//#include "/usr/include/sys/types.h"
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <complex>
#include <iostream>

#include <RLexact.h>
#include <cnr.h>

#include <mpi.h>
int rank, nprocs, gs_rank;

/* Functions declared in this file */
#ifdef LANCZOS 
void Solve_Lanczos();
#endif  /* LANCZOS */
#ifdef MATRIX
void Solve_Matrix();
#endif /* MATRIX */
void allocate();
void deallocate();

/* Functions declared elsewhere */
extern void BuildTables();
extern void BuildCycle(long long *);
extern unsigned long long FillUnique(long long, int);
extern void FillUniqueObservables();
extern long long intro();
extern void time_stamp(time_t*, long long, const char*);
extern void outro();
extern void fatalerror(const char *, long long);
extern void Warning(const char *, long long);
extern void LogMessageChar(const char *);
extern void LogMessageInt(long long);
extern void LogMessageImag(long long);
extern void LogMessageCharDouble(const char *, double);
extern void LogMessageCharInt(const char *, long long);
extern void LogMessageChar3Vector(const char *, double, double, double);
extern void OutMessageChar(const char *);
extern void WriteResults(long long);
extern void WritehmQ(long long *);
extern void WriteState(const char*,komplex*);
extern void WriteStates(komplex**);
extern void WriteGSdata(double,long long*);
extern void WriteGSstate(komplex*);
extern void WriteQvalue(long long *);
extern void ReadGSdata(double*,long long*,komplex*);
extern void ReadGSenergy(double*, long long *);
#ifndef FIND_MAG
extern void  findmaggs();
extern void WriteMaggs(long long *);
#endif /*M_SYM*/
#ifdef MATRIX
extern double Matrix_gs(komplex**, long long*, long long*, komplex*);
//extern void CrossMatrix(long long*); //out of order, SJ 270616
extern void MakeSparse();
#endif  /* MATRIX */
#ifdef LANCZOS
extern double LowestLanczos(long long*, komplex*, long long*, long long);
extern void CrossLanczos(long long*);
extern void MakeSparse();
#endif  /* LANCZOS */
extern void InitSym();
extern long long ReadUnique(long long, int);
extern void WriteUnique(long long);
extern void WriteUniqueObservables();
extern void ReadUniqueObservables();


/* Global variables read from the input file */
long long    Nspins;
       /* Number of spins in the system */
#ifdef MOTIVE
  long long Nspins_in_uc;
  double **spin_positions;
  long long Trans_Qmax[3];
#else
  long long Trans_Qmax = {1,1,1};
#endif //MOTIVE
long long    Ncoup;
       /* Actual number of couplings */
long long    Ncoupstr;
       /* Actual number of coupling strengths */
#ifdef RING_EXCHANGE
long long    Nring;
       /* Actual number of ring couplings */
long long    Nringstr;
       /* Actual number of ring coupling strengths */
#endif /* RING_EXCHANGE */
long long    Nsym;
       /* Total number of symmetries in use */
long long    Nsymadd;
       /* Number of added symmetries in use */
long long    symlist[NSYM];  /* Symmetries to be used in problem */
long long    **symadd;  /* Specification of additional
		     symmetries to be used in problem */
long long Ndimensions;
       /* How many dimensions are used */
long long *TransIds;
       /* Index of the translation symmetries in symlist */
long long    Nq_choice;
       /* number of symmetry vectors to be chosen */
long long    **q_choice;
       /* List of vectors of chosen symmetries */
long long    q_gs[NSYM];
       /* Symmetries of the ground state */
long long    twom;
       /* Two times the magnetization value (an integer!) */
double mstart, mend;
#ifndef M_SYM
double h;
       /* Magnitude of the magnetic field */
double hstart, hend, hstep;
       /* Limits for field values to be used */
double field[3];
       /* Three (normalized) components of the magnetic field */
#endif /* M_SYM */
double Jzz[NCOUP];
       /* z value of each coupling; Hz=Jz Sz1 Sz2 */
double Jxy[NCOUP];
       /* xy value of each coupling; Hxy=Jxy (Sx1 Sx2 + Sy1 Sy2) */
double Janis[NCOUP];
       /* xy-anisotropy of each coupling; Ha=Ja(Sx1 Sx2-Sy1 Sy2) */
#ifdef RING_EXCHANGE
  double Jr[NRING];
	/* value of ring exchange H_r = Jr sum_{ijkl in ring} (s_i . s_j) (s_k . s_l) + (s_i . s_l) (s_j . s_k) - (s_i . s_k) (s_j . s_l) */
  long long   ring_coup[NRING][4];
        /* Table of spin quadruplets coupled together in rings */
#endif /* RING_EXCHANGE */
#ifdef DIPOLE
double Jdip[NCOUP];
       /* Strength of dipole interaction (will be 1/r^3 or 0) */
double geom_13[NCOUP];
       /* geometrical factor */
double r_vector[NCOUP][3];
       /* Distance and direction between to spins */
#endif /* DIPOLE */
long long    hamil_coup[NCOUP][2];
       /* Table of spin pairs coupled together in pairs */ 
double Ritz_conv;
	/* Variables controlling the Lanczos algorithm */

/* Global variables generated by the program */
long long     Nunique;
       /* Actual number of different unique states */
long long    Nuniq_k;
       /* Number of allowed unique states for given value of k[] */
long long    Nsymvalue[NSYM]; 
       /* Number of different symmetry values for each symmetry (index) */
long long    *Nocc, *Nocc_0; 
       /*Number of a times a unique returns to itself under all translations through the system; Nocc = Nspins/Periodicity */
unsigned long long *unique;
       /* Unique Ising states (not connected by symmetries) */
long long    Nu2;  
       /* Lowest power of two larger than Nunique */
#ifdef MATRIX
long long    *uniq_k;  // BUGGED: NUNIQUE NOT AVAILABLE
long long Longest_Matrix; //Variable for deallocation at the end, stores first number of uniques
#else
long long    uniq_k[1];
#endif
       /* uniq_k is the index from reduced unique (at a given k) to total unique */

long long   Nelem;
       // Number of matrix elements
#ifndef M_SYM
long long    *mag;
       /* m-value for the unique states, TODO: should be int */ 
#endif /* M_SYM */
komplex **hamilton;
       /* The Hamiltonian matrix, dynamically allocated */
komplex *vec1,*vec2,*vec3;
       /* State vectors, dynamically allocated */
komplex *gs;
       /* Ground state, dyn. all. */
komplex *szxygs, *smgs, *spgs; /*vectors for use in calculation of dynamical correlation functions */
komplex *evec, *tmp, *tmp2, *shadow; // shadow is always alias for one of the other vectors
double cosine[NSPINS],sine[NSPINS];
       /* Lookup tables */
double sqroot[2*(NSPINS+1)*NSPINS];
       /* Lookup table */
double gs_energy;
       /* Energy of the ground state */
double szqlength;
long long mode; /* the mode of the program: 0 is normal, 1 is "find groundstate for
	     specified q, 2 is reconstruct the groun dstate, 3 is read gs-energy and q 
		 from file and find szz for specified q */
long long unimode; /* the "unique mode" of the program: 0 is normal, 1 is find unique tables
		      and write to disk, 2 is read unique tables from disk */
int spinflip_present; //This is 1 is spinflip is present and 0 if it is not.
int spinflip_number; /* If spin flip symmetry is present this has a value equal the corresponding index in the symmetry list; otherwise -1 */ 
int spinflip_GSvalue; /* If spin flip symmetry is present, this is its value in the Ground State */ 
		      
/* The input filename */
char *infile_name;
bool name_on_commandline=false;
FILE *gscoinfile;

/* Global output files */
#ifdef FIND_CROSS
FILE   *outfilezz;

  #ifndef FIND_CROSS_PM
  FILE  *outfilexx, *outfileyy;
  #endif
  #ifdef FIND_CROSS_PM
  FILE  *outfilepm, *outfilemp;
  #endif

       /* Output file for storing data */
#endif /* FIND_CROSS */
FILE   *outfile;
FILE   *logfile;

/* Output variables */
double *energies;
#ifdef FIND_MAG
double *magnetisation;
#endif /* FIND_MAG */
#ifdef MATRIX 
double *cross;
#endif /* Matrix*/
#ifdef LANCZOS
double *cross;
//double cross[MAX_LANCZOS];
#endif /* LANCZOS */
#ifdef FIND_MAG
double maggs;
#endif /*M_SYM*/

/* ---------------------------------------------------------------------- */

int main(int argc, char* argv[])
{
  gs_rank = 0;
  rank = 0; //Don't know if this is necessary, but it's a nice fail safe. -ABP
  nprocs = 1;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

 time_t time_total,time_single0, time_single, time_single2;
 infile_name=(char*)malloc(30*sizeof(char));

 if (argc>1) {
   infile_name=argv[1];
   name_on_commandline=true;
 }


 if (intro() == -1)
   return 1;

 srand(time(NULL)); // WARNING: DECOMMENT BEFORE USE
 time_stamp(&time_total,START,"diagonalization \n");

#ifdef VERBOSE_TIME_LV1
 time_stamp(&time_single,START,"filling basic arrays");
#endif

  BuildTables();
  InitSym();

 #ifdef M_SYM
  LogMessageChar("With M-symmetry \n");
 #else
  LogMessageChar("Without M-symmetry \n");
 #endif

#ifdef M_SYM
 
 // First we must find the maximum number of unique states for any m.
 // This is always the number of uniques for the lowest absolute m-value.
#ifdef VERBOSE
 LogMessageChar("M_SYM encountered 1st \n");
 LogMessageCharInt("Unique mode is: ", unimode);
 LogMessageCharInt("Mode is: ",mode);
 LogMessageCharInt("Nspins%2 is: ", Nspins%2);
 LogMessageCharInt("twom is: ", twom);
 LogMessageCharInt("mstart = ",mstart);
 LogMessageCharInt("mend = ",mend);
 LogMessageChar("\n");
#endif //VERBOSE

 if ((mstart>0) && (mend >0)) {
   if ((unimode==UNIMODER) && (rank == 0)) { //UNIMODER Reads from file.
                                             //Not tested with MPI since perl
                                             //scripts are lost.
     Nunique = ReadUnique((long long)(mstart*2),1);
   } else {
//	 LogMessageChar("FillUnique 1 \n");
     Nunique = FillUnique ((long long)(mstart*2),1);
   }
 } else {
   if ((mstart<0) && (mend<0)) {
     if ((unimode==UNIMODER)&&(rank==0)) {
       Nunique = ReadUnique((long long)(mend*2),1);
     } else {
//	 LogMessageChar("FillUnique 2 \n");
       Nunique = FillUnique ((long long)(mend*2),1);
     }
   } else {
     if (Nspins%2==0) {
//     LogMessageCharInt("Nspins%2 loop entered, unimode ", unimode);
       if ((unimode==UNIMODER) && (rank==0)) {
//	 LogMessageChar("ReadUnique 3 \n");
	 Nunique = ReadUnique((long long)(0),1);
       } else {
//	 LogMessageChar(" FillUnique 3 \n");
	 Nunique = FillUnique (0,1); //twom=0 if this loop is entered
       }
     } else {
       if ((unimode==UNIMODER) && (rank==0)){
//	 LogMessageChar("ReadUnique 4 \n");
	 Nunique = ReadUnique((long long)(1),1);
       } else {
//	 LogMessageChar("FillUnique 4 \n");
	 Nunique = FillUnique (1,1);
       }
     }
   }
 }
   
#else //NOT M_SYM
#ifdef VERBOSE
 LogMessageChar("M_SYM not encountered \n");
 LogMessageCharInt("Unique mode is: ", unimode);
 LogMessageCharInt("Mode is: ",mode);
 LogMessageCharInt("Nspins%2 is: ", Nspins%2);
 LogMessageCharInt("h is: ", h);
 LogMessageCharInt("hstart is ",hstart);
 LogMessageCharInt("hend is ",hend);
 LogMessageCharDouble("in steps ",hstep);
 LogMessageChar("\n");
#endif //VERBOSE

 if ((unimode==UNIMODER)&&(rank==0)) {
   Nunique = ReadUnique(0,1);
 } else {
   Nunique = FillUnique(0,1);
 }
#endif /* M_SYM */
// LogMessageChar("Unique mode OK \n");
// LogMessageCharInt("Memory needed for vectors is ", (Nunique*4*sizeof(komplex)));
// LogMessageChar("\n");

 allocate();
#ifdef MATRIX
 Longest_Matrix = Nunique;
#endif /*MATRIX*/

#ifdef VERBOSE_TIME_LV1
 time_stamp(&time_single,STOP,"Longest_Matrix allocated ");
#endif
 
#ifdef M_SYM
//LogMessageChar("M_SYM encountered \n");
for (twom=(long long)(2*mstart); twom<= (long long)(2*mend); twom=twom+2) 
{

  #ifdef VERBOSE_TIME_LV1
    LogMessageCharInt("\n Now treating the m = ",twom/2);
    LogMessageChar("subspace: \n\n");
    time_stamp(&time_single0,START,"filling arrays for one m");
  #endif

  if ((unimode==UNIMODER)&&(rank==0)) {
   Nunique=ReadUnique(twom,0);
  } else {
   Nunique=FillUnique(twom, 0);
   FillUniqueObservables();
  }

  #ifdef VERBOSE_TIME_LV1
     time_stamp(&time_single0,STOP," ");
     time_stamp(&time_single0,START,"calculating for one m");
  #endif

#else /*Not M_SYM*/

#ifdef VERBOSE_TIME_LV1
   time_stamp(&time_single0,START,"filling arrays of observables for all h \n");
#endif

   if ((unimode==UNIMODER)&&(rank==0)) {
    LogMessageChar("The mode is UNIMODER \n");
     ReadUniqueObservables();
     Nunique=ReadUnique(0,0);
   } else {
     Nunique=FillUnique(0,0); // TODO: CHECK THIS: slightly disgusting:
                              // variable is never used - FillUnique takes care
                              // of both M_SYM set and unset
     LogMessageChar("\n FillUnique is done \n");
     FillUniqueObservables();
     LogMessageChar("FillUniqueObservables is done \n");
   }
#ifdef VERBOSE_TIME_LV1
   time_stamp(&time_single0,STOP," ");
#endif

   for (h=hstart; h<= hend; h+=hstep) 
   {

#ifdef VERBOSE_TIME_LV1
     LogMessageCharDouble("\n Now treating the h = ",h);
     LogMessageChar("subspace \n\n");
     time_stamp(&time_single0,START,"calculating for one h");
#endif

#endif /* M_SYM */

#ifdef VERBOSE_TIME_LV1
     LogMessageCharInt("\n Nunique = ",Nunique);
#endif //VERBOSE_TIME_LV1  

     if ((unimode==UNIMODEW)&&(rank==0))
     {
       WriteUnique(twom);
#ifndef M_SYM
       long long q_write[NSYM];
       for (int i = 0; i < NSYM; i++)
       {
         q_write[i]=0;
       }
       BuildCycle(q_write);
       WriteUniqueObservables();
#endif /* not M_SYM */
     } 
     else 
     {
#ifdef MATRIX
       if (rank == 0) //TODO Also MPI on Matrix mode - ABP 2025-03-13
       Solve_Matrix();
#endif /* MATRIX */
#ifdef LANCZOS
       Solve_Lanczos();
#endif /* LANCZOS */

#ifdef VERBOSE_TIME_LV1
       time_stamp(&time_single0,STOP,"one m/h ");
       LogMessageChar("\n");
#endif

     }
}

time_stamp(&time_total, STOP, "\n Total execution");
outro();
deallocate();
LogMessageChar("deallocated correctly!");

MPI_Barrier(MPI_COMM_WORLD);
MPI_Finalize();
if (rank == 0) OutMessageChar("\n End of diagonalization program RLexact.\n");
return 0;
} //end main()

/* ----------------------------------------------------------------------------------*/

#ifdef LANCZOS
void Solve_Lanczos() {
  /* Main routine for calculations using the Lanczos method */
  time_t time_single0, time_single, time_single2, time_makesparse;
  long long vec[NSYM],sym,Nener;
  long long i,j;
  long long *q = &vec[0];
  double etmp;

  vec1 = tmp;
  vec2 = tmp2;
  vec3 = shadow;

  MPI_Barrier(MPI_COMM_WORLD);
  if (mode == MODEW || mode == MODEN){
    #ifdef VERBOSE_TIME_LV1
      time_stamp(&time_makesparse,START,"writing to file");
    #endif
        // Make sparse matrix (writing to file)
    LogMessageChar("\nwriting sparse matrix to file: begin");
    if (rank==0) MakeSparse();
    LogMessageChar("writing sparse matrix to file: end\n");
    #ifdef VERBOSE_TIME_LV1
      time_stamp(&time_makesparse,STOP, "..");
    #endif
  }
  MPI_Barrier(MPI_COMM_WORLD);

  #ifdef VERBOSE_TIME_LV1
    time_stamp(&time_single0,START,"dealing with the ground state");
    time_stamp(&time_single,START,"finding the ground state");
  #endif
        
  // Run Lanczos to find eigenenergies and identify the ground state
  if (mode==MODEGS || mode==MODEN) {
    if (Nq_choice > 0)
    {
      if (rank==0)
      {
        gs_energy=LARGE_NUMBER;
        for (j=0; j<Nq_choice; j++)
        {
          q=&q_choice[j][0];
          LogMessageChar("chosen q, q=(");
          for (i=0; i<Nsym; i++) 
            LogMessageCharInt(" ",q[i]);

#ifndef M_SYM
          LogMessageCharDouble(" H= ",h);
#endif /* M_SYM */	   

          LogMessageChar(") \n");
          BuildCycle(q);
          etmp=LowestLanczos(q,NULL,&Nener,NORMAL);

#ifdef WRITE_ENERGIES
          WritehmQ(q);
          WriteResults(Nener);
#endif /* WRITE_ENERGIES */

          if (etmp<gs_energy)
          {
            gs_energy=etmp;
            for (sym=0; sym<Nsym; sym++){
              q_gs[sym] = q[sym];
            }
            //      for (i=0; i<Nunique; i++)
            //	gs[i] = tmp[i];
          }
        }
      }
      MPI_Bcast(&gs_energy, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
      MPI_Bcast(q_gs, Nsym, MPI_LONG_LONG_INT, 0, MPI_COMM_WORLD);      
    }
    else //Nq_choice <= 0 -> Search all q
    {
      gs_energy=LARGE_NUMBER;
  
      int mpi_q_count = 0;
      QLOOP_BEGIN
        
        if (mpi_q_count%nprocs == rank)
        {

#ifdef VERBOSE_TIME_LV2
          LogMessageChar("\nSolve_Lanzcos: GS q-loop, q=(");
          for (i=0; i<Nsym; i++) 
            LogMessageInt(q[i]);
          LogMessageChar(") \n");
          time_stamp(&time_single2,START,"Ground state search for one q ");
#endif /* VERBOSE_TIME_LV2 */

          BuildCycle(q);
          etmp=LowestLanczos(q,NULL,&Nener,NORMAL); 

#ifdef WRITE_ENERGIES
          WritehmQ(q);
          WriteResults(Nener);
#endif /* WRITE_ENERGIES */

          if (etmp<gs_energy)
          {
            gs_energy=etmp;
            for (sym=0; sym<Nsym;sym++){
              q_gs[sym] = q[sym];
            }
            //      for (i=0; i<Nunique; i++)
            //	gs[i] = tmp[i];
            //	gs[i] = evec[i];
          }

#ifdef VERBOSE_TIME_LV2
          time_stamp(&time_single2,STOP," ");
#endif
        }
      mpi_q_count++; 
      QLOOP_END

      if (mode==MODEGS) {
        Warning("YOU SHOULD SPECIFY Q-VALUES TO DETECT FOR MODEGS",0);
      }
  
      #ifdef VERBOSE_TIME_LV1
        time_stamp(&time_single,STOP,"finding the ground state ");
      #endif

      struct {
        double gs_energy;
        int rank;
      } send_data, recv_data;

      send_data.gs_energy = gs_energy;
      send_data.rank = rank;

      MPI_Allreduce(&send_data, &recv_data, 1, MPI_DOUBLE_INT, MPI_MINLOC,
          MPI_COMM_WORLD);

      gs_energy = recv_data.gs_energy;
      gs_rank = recv_data.rank;
      MPI_Bcast(q_gs, Nsym, MPI_LONG_LONG_INT, gs_rank, MPI_COMM_WORLD);
      LogMessageCharDouble("GS_energy after Allreduce", gs_energy);
      

    }  /* end if (Nq_choice > 0) */
	  
	  if ((mode == MODEGS)&&(rank==0))
          {
	     // End of finding the ground state, write ground state and q-symmetry to file:
	     WriteGSdata(gs_energy,q_gs);
	     //Done with modegs
	  }
	
  } /* End of if(mode == MODEGS || mode == MODEN) */
	
	 // Now, reconstruct the groundstate.
#ifdef VERBOSE_TIME_LV1
         time_stamp(&time_single,START,"reconstructing the ground state ");
#endif

    if (mode==MODEN || mode==MODERC) {
        
       if ((mode==MODERC)&&(rank==0)){
	 //Read the ground state energy and q-vector from file
	 ReadGSenergy(&gs_energy,q_gs);
       }
       if (gs_rank == rank){
       BuildCycle(q_gs);
       LowestLanczos(q_gs,gs,&Nener,RECONSTRUCT);
       }
      MPI_Bcast(gs, Nunique, MPI_C_DOUBLE_COMPLEX, gs_rank,
          MPI_COMM_WORLD); 
      MPI_Bcast(&Nener, 1, MPI_LONG_LONG_INT, gs_rank, MPI_COMM_WORLD);
#ifdef WRITE_STATES
        // print the groundstate vector to outfile
      if (gs_rank == rank){
        WriteQvalue(q_gs);
        WriteState("Groundstate",gs);
        }
      MPI_Barrier(MPI_COMM_WORLD);
#endif /* WRITE_STATES */
      if ((mode == MODERC)&&(rank==0)){
		//write reconstructed ground state to .gs file:
		WriteGSstate(gs);
		//done with moderc		  
	  }

#ifdef VERBOSE_TIME_LV1
         time_stamp(&time_single,STOP,"reconstructing the ground state ");
#endif
  }

    #ifdef TEST_SPINFLIP
      LogMessageCharInt("\nspinflip_number=",spinflip_number);
      LogMessageCharInt("spinflip_present =",spinflip_present);
    #endif  

  if (spinflip_present == 1)
  {
    spinflip_GSvalue = q_gs[spinflip_number];
  }
  else 
  {
    spinflip_GSvalue = Nspins+1;  // Set it to a never-used q-value
    spinflip_number = 0;          // Need this to do the indexing, even if spin
                                  // flip is not chosen
  }

#ifdef VERBOSE_TIME_LV1
    time_stamp(&time_single0,STOP,"dealing with the ground state ");
#endif

#ifdef FIND_MAG
  if(mode==MODEGS || mode==MODEN){
    //findmaggs();//Parameters are not necessary to state explicitly here, since these are only global parameters, available for all files.
    //WriteMaggs(q);
  }
#endif /*M_SYM*/

#ifdef FIND_CROSS

#ifdef VERBOSE_TIME_LV1
      time_stamp(&time_single,START,"Cross sections");    
#endif

if ((mode==MODEQ)&&(rank==0)) {
      ReadGSdata(&gs_energy,q_gs,gs);
  #ifdef WRITE_STATES
        // print the groundstate vector to outfile
        WriteState("Groundstate",gs);
  #endif /* WRITE_STATES */
        q=&q_choice[0][0];
  #ifdef VERBOSE_TIME_LV2
        time_stamp(&time_single2,START,"Cross section for one chosen q-value");
        LogMessageChar("\nCrossection q-loop, q=(");
        for (i=0; i<Nsym; i++) 
  	LogMessageCharInt(" ",q[i]);
        LogMessageChar(") \n");
  #endif
        BuildCycle(q);


  #ifdef TEST_APPLYSZQ
        LogMessageChar("\nCalling CrossLanczos \n \n");
  #endif //TEST_APPLYSZQ 
        

        CrossLanczos(q);
  #ifdef VERBOSE_TIME_LV2
        time_stamp(&time_single2,STOP," ");
  #endif

} else if (mode==MODEN) {

  // Set maxloop for translation symmetries before QLOOP
  // Nsymvalue[transID].
  //
  // Depending on symmetries, optimization can be made using selection rules
  // like SPINFLIP below.

  long long Nqvalue[Nsym];
  for (int i = 0;i<Nsym;i++) Nqvalue[i] = Nsymvalue[i];
  for (int i = 0;i<Ndimensions;i++) Nqvalue[TransIds[i]] *= Trans_Qmax[i];

  for (int i=0; i<Nsym;i++){
    LogMessageCharInt("\n",i);
    LogMessageCharInt("Nqvalue",Nqvalue[i]);
    LogMessageCharInt("Nsymvalue",Nsymvalue[i]);
  }
  
  /*for (int i=0; i<Ndimensions; i++){
   //Trans_symvalue_dummy[TransIds[i]] = Nsymvalue[TransIds[i]];
   Nsymvalue[TransIds[i]] *= Trans_Qmax[i]; //Nsymvalue bruges i loop for AppSz
  }*/
  int mpi_q_count=0;
  for (sym=0;sym<Nsym;sym++) q[sym]=0;
  sym=0;
  while(q[Nsym-1]<Nqvalue[Nsym-1]){
  //QLOOP_BEGIN

    if (mpi_q_count % nprocs == rank) {
#ifdef TEST_SPINFLIP
    LogMessageCharInt("spinflip_GSvalue =",spinflip_GSvalue);
    LogMessageCharInt("spinflip_number =",spinflip_number);
    LogMessageCharInt("q[spinflip_number] =",q[spinflip_number]);
#endif
    if (spinflip_GSvalue != q[spinflip_number] ) //consistent with spinflip not present also
    {
#ifdef VERBOSE_TIME_LV2
      time_stamp(&time_single2,START,"Cross section for one q-value in loop");    
      LogMessageChar("\nCrossection q-loop, q=(");
      for (i=0; i<Nsym; i++) 
      {
        LogMessageCharInt(" ",q[i]);
      }
      LogMessageChar(") \n");

#endif
      BuildCycle(q);

#ifdef TEST_APPLYSZQ
      LogMessageChar("\nCalling CrossLanczos: \n \n");
#endif //TEST_APPLYSZQ 

      CrossLanczos(q);

#ifdef VERBOSE_TIME_LV2
      time_stamp(&time_single2,STOP," ");
#endif
    }
    }
    mpi_q_count++;
    //QLOOP_END
    for (sym=0;++q[sym]==Nqvalue[sym]; ){
      if (sym<Nsym-1) q[sym++]=0;
    }
  }
    //for (int i=0; i <Ndimensions; i++) Nsymvalue[TransIds[i]] /= Trans_Qmax[i];
  }
#ifdef VERBOSE_TIME_LV1
      time_stamp(&time_single,STOP,"\nCross sections ");
#endif
#endif /* FIND_CROSS */

  return;
 }
#endif  /* LANCZOS */

/* ---------------------------------------------------------------------------- */

#ifdef MATRIX
void Solve_Matrix()
 {
  /* Main routine for calculations using the matrix method. 
  The code works but report memory deallocation error at the end.*/
  time_t time_single;
  long long vec[NSYM],sym;
  long long i,j;
  double etmp;
  long long *q = &vec[0];

  time_stamp(&time_single,START,"finding the ground state");

  //Find elements of the hamiltonian and write them to file
  MakeSparse();

  if (Nq_choice > 0)
  {
   gs_energy=LARGE_NUMBER;
   for (j=0; j<Nq_choice; j++)
   {
    q=&q_choice[j][0];
#ifdef TEST_GS_SEARCH
    LogMessageChar("TEST_GS1: chosen q, q=(");
    for (i=0; i<Nsym; i++) 
      LogMessageInt(q[i]);
    LogMessageChar(") \n");
    hamilton[1][1]=zero;
    LogMessageChar("Hamiltonian accessed. \n");
#endif  /* TEST_GS_SEARCH */
    gs_energy=Matrix_gs(hamilton,uniq_k,q,gs);
#ifdef WRITE_ENERGIES
     WritehmQ(q);
     WriteResults(Nuniq_k);
#endif /* WRITE_ENERGIES */
#ifdef WRITE_STATES
     WriteStates(hamilton);
#endif /* WRITE_STATES */
   }
  }
  else
  {
   gs_energy=LARGE_NUMBER;
   QLOOP_BEGIN
	  // hamilton = kmatrix(1, Nunique, 1, Nunique); //Allocating each time to avoid bug (THIS IS A TEMPERARY FIX)


 #ifdef TEST_GS_SEARCH
       LogMessageCharInt("TEST_GS2: GS q-loop, 2*m =",twom);
       LogMessageChar(", q=(");
       for (i=0; i<Nsym; i++) 
         LogMessageInt(q[i]);
       LogMessageChar(") \n");
    hamilton[1][1]=zero;
    LogMessageChar("Hamiltonian accessed. \n");
#endif  /* TEST_GS_SEARCH */
       etmp=Matrix_gs(hamilton,uniq_k,q,evec); 
       if (etmp < LARGE_NUMBER)  /* else: illegal symmetry combination */
       {
#ifdef WRITE_ENERGIES
        WritehmQ(q);
        WriteResults(Nuniq_k);
#endif /* WRITE_ENERGIES */
#ifdef WRITE_STATES
        WriteStates(hamilton);
#endif /* WRITE_STATES */
       }
       if (etmp<gs_energy)
       {
         gs_energy=etmp;
         for (sym=0; sym<Nsym; q_gs[sym]=q[sym++]);
		 for (i = 0; i < Nunique; i++) gs[i] = evec[i];
       }

	   QLOOP_END

  }  /* if (Nq_choice != 0) */
  time_stamp(&time_single,STOP," ");
  if (spinflip_number > -1)
  {
    spinflip_GSvalue = q[spinflip_number];
    LogMessageCharInt("Ground State spinflip eigenvalue is:",spinflip_GSvalue);
    LogMessageChar("\n");
  }
  else 
  {
    spinflip_GSvalue = Nspins+1;  // Set it to a never-used q-value
    //spinflip_number = 0;          // Need this to do the indexing, even if spin flip is not chosen
  }

#ifdef FIND_CROSS
  QLOOP_BEGIN
     if (spinflip_GSvalue != q[spinflip_number] )  
     {
       time_stamp(&time_single,START,"Cross section for one q-value");
        //CrossMatrix(q); //Does not work
        time_stamp(&time_single,STOP," ");
     }
  QLOOP_END
#endif /* FIND_CROSS */
  return;
 }
#endif  /* MATRIX */

//----------------------------

void allocate()
 {

#ifdef LANCZOS
	 /* Allocate large vectors */
	 gs = kvector(0, Nunique - 1);
	 tmp = kvector(0, Nunique - 1);
	 tmp2 = kvector(0, Nunique - 1);

	 /* allocate unique tables */
	 Nocc = (long long*)malloc(sizeof(long long)*Nunique);
   Nocc_0 = (long long*)malloc(sizeof(long long)*Nunique);
   unique = (unsigned long long*)malloc(sizeof(unsigned long long)*Nunique);
#ifdef FIND_EIGENSTATE
  szxygs=     kvector(0,Nunique-1); // this is _always_ needed, either for cross section, or as 3 vector in lanczos algorithm
  smgs=     kvector(0,Nunique-1); //
  spgs=     kvector(0,Nunique-1);
  shadow = szxygs;
#else /* FIND_EIGENSTATE */
  shadow = gs; // gs is not needed to hold groundstate: We dont want it
#endif /* FIND_EIGENSTATE */
  energies = dvector(0,MAX_LANCZOS-1);
#ifndef M_SYM
  mag = (long long*)malloc(sizeof(long long)*Nunique);
#endif /*M_SYM*/
#ifdef FIND_MAG
  magnetisation = dvector(0,MAX_LANCZOS-1);
#endif /*FIND_MAG*/
#ifdef FIND_CROSS
    cross = dvector(0,MAX_LANCZOS-1);
#endif /* FIND_CROSS */
#endif  /* LANCZOS */

#ifdef MATRIX
	/* Allocate large vectors */
  gs = kvector(0, Nunique);
  tmp = kvector(0, Nunique - 1);
  tmp2 = kvector(0, Nunique - 1);

	/* allocate unique tables */
  Nocc = (long long*)malloc(sizeof(long long)*Nunique);
  unique = (unsigned long long*)malloc(sizeof(unsigned long long)*Nunique);
  evec = kvector(0, Nunique);
  energies = dvector(0,Nunique-1);
  uniq_k=(long long*)malloc(sizeof(long long)*Nunique);
  hamilton = kmatrix(1, Nunique, 1, Nunique);

#ifdef TEST_ALLOCATE
  MessageCharInt("Hamiltonian matrix defined, size; ", Nunique);
  MessageChar("\n");
  hamilton[1][1]=zero;
  MessageChar("Hamiltonian matrix accessed \n");
#endif /* TEST_ALLOCATE */

#ifndef M_SYM
  mag = (long long*)malloc(sizeof(long long)*Nunique);
#endif /* FIND_MAG */
#ifdef FIND_MAG
  magnetisation = dvector(0,Nunique-1);
#endif /*FIND_MAG*/
#ifdef FIND_CROSS
    cross =    dvector(1,Nunique);
#endif  /* FIND_CROSS */
#endif  /* MATRIX */

  return;
 }

void deallocate()
 {
  /* Deallocate large vectors */
#ifdef LANCZOS
	 freekvector(gs, 0, Nunique - 1);
	 freekvector(tmp, 0, Nunique - 1);
   freekvector(tmp2, 0, Nunique - 1);
	 freekvector(evec, 0, Nunique - 1);
   free(Nocc);
   free(Nocc_0);
   free(unique);
   freekvector(szxygs, 0, Nunique - 1);
   freekvector(smgs, 0, Nunique - 1);
   freekvector(spgs, 0, Nunique - 1);
   freedvector(energies,0,MAX_LANCZOS-1);
#ifdef FIND_MAG
  freedvector(magnetisation,0,MAX_LANCZOS-1);
#endif /* M_SYM */
#ifdef FIND_CROSS   
    freedvector(cross,0,MAX_LANCZOS-1);
#endif /* FIND_CROSS */
#endif  /* LANCZOS */

#ifdef MATRIX
	freekvector(gs, 0, Longest_Matrix );
	freekvector(tmp, 0, Longest_Matrix - 1);
  freekvector(tmp2, 0, Longest_Matrix - 1);
	freekvector(evec, 0, Longest_Matrix);

  free(Nocc);
  free(unique);
  freekmatrix(hamilton, 1, Longest_Matrix, 1, Longest_Matrix);
  freedvector(energies, 0, Longest_Matrix-1); //There is something wrong with this vector can't deallocate
  free(uniq_k);

  #ifdef FIND_CROSS
	freekvector(szxygs, 0, Longest_Matrix - 1);
  freedvector(cross, 1, Longest_Matrix );
  #endif /* FIND_CROSS */

#ifndef M_SYM
  free(mag);
#endif /* M_SYM */

#ifdef FIND_MAG
  freedvector(magnetisation, 0, Longest_Matrix - 1);
#endif /* FIND_MAG */

#endif  /* MATRIX */
#ifdef MOTIVE
  for (int i = 0; i <Nspins_in_uc; i++) free(spin_positions[i]);
  free(spin_positions);
#endif //MOTIVE
  return;
 }
