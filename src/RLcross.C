/* Program file RLcross.C - 
 * Calculating cross sections 
 * Last change: SJ 09.11.16
 *
 ============================================
 *
 * RLexact: The exact diagonalization package
 * Christian Rischel & Kim Lefmann, 26.02.94  
 * Version 4.0, September 2017
 *
 ============================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <complex>
#include <RLexact.h>
#include <nr.h>
#include <math.h> //TODO: Remove when tables implemented /ABP

#ifdef FIND_CROSS

/* Functions declared elsewhere */
extern unsigned long long SymOp(long long, unsigned long long);
extern double LowestLanczos(long long*, komplex*, long long*, long long);
extern void WriteCross(long long, long long*, long long);
extern unsigned long long FindUnique(unsigned long long, int *);
extern long long LookUpU(unsigned long long);
extern void LogMessageChar(const char *);
extern void LogMessageInt(long long);
extern void LogMessageImag(const double, const double);
extern void LogMessageKomlex(const komplex);
extern void LogMessageCharKomplex(const char *, const komplex);
extern void LogMessageCharDouble(const char *, double);
extern void LogMessageCharInt(const char *, long long);
extern void LogMessageChar3Vector(const char *, double, double, double);
extern void fatalerror(const char*, long long);

/* Functions declared in this file */
void ApplySzq(long long*);
double lengthofvector(komplex*);
#ifndef M_SYM
void ApplySmp(long long *, long long, komplex *);
#endif
#ifdef LANCZOS
void CrossLanczos(long long *); 
#endif /* LANCZOS */

#ifdef NEVER //doesnt work, SJ 270616
void ApplySmpMsym(long long *, long long);
#ifdef MATRIX
void CrossMatrix();
int sym;
#endif /* MATRIX */
bool NonZero(unsigned long long, long long*);
#endif /*NEVER*

/* Global variables defined in RLexact.c */
#ifdef M_SYM
  extern long long twom;
#endif /* M_SYM */

extern long long Nspins,Nunique,hamil_coup[NCOUP][2];
extern long long *TransIds;
extern long long Ndimensions;
extern long long Nspins_in_uc;
extern double **spin_positions;
extern long long Nuniq_k, Nsym;
extern long long uniq_k[];
extern long long *Nocc, *Nocc_0;
extern long long Nsymvalue[NSYM];
extern double cosine[],sine[],sqroot[];
extern unsigned long long *unique;
extern komplex *gs;
extern double gs_energy;
extern komplex *szxygs, *smgs, *spgs;
extern double szqlength;
extern long long symlist[NSYM];
extern long long q_gs[NSYM];
extern double *cross;
// double *energies;
//FILE *outfilezz, *outfilepm, *outfilemp;

/* Regional variables defined here */
long long k[NSYM];


#ifdef LANCZOS
//void CrossLanczos(int symvalue[NSYM])
void CrossLanczos(long long *symvalue) //(Note: symvalue =qvector)
{
  long long j,r,Nener;
  int maxflag;

#ifdef TEST_CROSS
#ifdef M_SYM
  LogMessageCharInt("\n for twom =",twom);
#endif
  LogMessageCharInt("\nIn q = ",symvalue[1]);
  for (int i = 2;i<Nsym;i++) LogMessageCharInt(", ",symvalue[i]);
  LogMessageChar("\n");
  for (int i = 0; i < Nunique; i++)
  {
    LogMessageCharInt("unique",unique[i]);
    LogMessageCharDouble(", gs[i] =",real(gs[i]));
    LogMessageCharDouble("+ i",imag(gs[i]));
    LogMessageChar("\n");
  }
#endif // TEST_CROSS

  /* Sort out which symmetry elements are in real-space */
  /* For a start: Consider only periodic boundary conditions */
  /* Apply S+-(k),S-+(k),Sz(k) to groundstate */

#ifndef FIND_CROSS_PM //otherwise not needed
                      //initialize
  Nener =0;
  for (int j = 0; j < Nunique; j++)
  {
    spgs[j]=zero; 
    smgs[j]=zero; 
  }
#endif

  //how many cross sections can be calculated?
#ifdef M_SYM
  maxflag = 1;
#endif
#ifndef M_SYM
  maxflag = 3;
#endif

  for (int flag = 0; flag < maxflag; flag++) //first SZZ, then SXX, then SYY
  { 
    //initialize
    Nener =0;
    for (int j = 0; j < Nunique; j++)
    {
      szxygs[j]=zero; 
    }
#ifdef TEST_CROSS
    LogMessageCharInt("\n\nFlag:", flag);
    LogMessageChar("\n\n");
#endif

    if (flag == 0) //SZZ
    {
      ApplySzq(symvalue);
    }
#ifndef M_SYM  

#ifndef FIND_CROSS_PM //find in terms of S^xx and S^yy
    if (flag == 1) //SXX, s^x |gs> = 1/2 (s^+_q |gs> + s^-_q |gs>)
    {     
      /* applysmp is calculated here, but also needed in flag==2 */
      ApplySmp(symvalue,0,spgs); //find s^+_q |gs>
      ApplySmp(symvalue,1,smgs); //find s^-_q |gs>

      for (int i = 0; i < Nunique; i++)
      {
        szxygs[i]=(1.0/(2.0))*(spgs[i]+smgs[i]);
      }
    }

    if (flag == 2) //SYY, s^y |gs> = 1/(2i) (s^+_q |gs> - s^-_q |gs>)
    {
      for (int i = 0; i < Nunique; i++)
      {
        szxygs[i]=(1.0/(2.0*I))*(spgs[i]-smgs[i]);
      }
    }
#endif //not FIND_CROSS_PM

#ifdef FIND_CROSS_PM //find in terms of S^+- and S^-+
    if (flag == 1) //SMP
    {
      ApplySmp(symvalue,0,szxygs); //find s^+_q |gs>
    }
    if (flag == 2) //SPM
    {
      ApplySmp(symvalue,1,szxygs); //find s^-_q |gs>
    }
#endif //FIND_CROSS_PM

#endif  //M_SYM

    szqlength=lengthofvector(szxygs); //for output in cross files

#ifdef TEST_CROSS
    LogMessageChar("\nCalling LowestLanczos\n");
    for (int j = 0; j < Nunique; j++)
    {
      LogMessageCharInt("Unique ", unique[j]);
      LogMessageCharDouble(", szxygs[j]=",real(szxygs[j]));
      LogMessageCharDouble("+i",imag(szxygs[j]));
      LogMessageCharDouble(", spgs[j]=",real(spgs[j]));
      LogMessageCharDouble("+i",imag(spgs[j]));
      LogMessageCharDouble(", smgs[j]=",real(smgs[j]));
      LogMessageCharDouble("+i",imag(smgs[j]));
      LogMessageChar("\n");
    }
#endif // TEST_CROSS

    LowestLanczos(symvalue,NULL,&Nener,CROSS); 
    /*This will do the actual work! When done, global variables 
     *energies and *Cross contains the relevant values for cross-section*/

    WriteCross(Nener,symvalue,flag);

#ifdef TEST_CROSS
    LogMessageChar("Data has been written to file\n");
#endif

  } /*for flag*/

  return;
}
#endif /* LANCZOS */


/* ApplySzq applies the Sz(q) operator to a state vector */
void ApplySzq(long long *q)
{
  long long i,n,cycle;
  //long long phase;
  unsigned long long state,gsstate,new_state;
  komplex res,factor, spin_pos_res;
  long long sym, T[NSYM],diffQ[NSYM];
  double phase; //Potentially int, if RLtables is implemented /ABP

  for (sym=0;sym<Nsym;sym++) 
  {
    diffQ[sym]=q[sym]-q_gs[sym];
    if (diffQ[sym]<0) 
    {
      diffQ[sym]+=Nsymvalue[sym];
    }
    if (symlist[sym]==SPIN_FLIP && diffQ[sym]==0)
    {
      for(i=0;i<Nunique;i++)
      {
        szxygs[i]=zero;
        return;
      }
    }

#ifdef TEST_APPLYSZQ
    LogMessageCharInt("ApplySzq: sym=",sym);
    LogMessageCharInt(", diffQ=",diffQ[sym]);
    LogMessageCharInt(", q=",q[sym]);
    LogMessageCharInt(", q_gs=",q_gs[sym]);
    LogMessageCharInt(", Nsym=",Nsym);
    LogMessageCharInt(", symlist=",symlist[sym]);
    LogMessageChar("\n");
#endif // TEST_APPLYSZQ
  } 

  for (i=0; i<Nunique; i++)
  {
    res=zero;
    gsstate = unique[i];
#ifdef TEST_APPLYSZQ
    LogMessageCharInt("\nNunique loop, i =",i);
    LogMessageCharInt("Groundstate coefficients for unique",gsstate);
    LogMessageCharDouble(" are",real(gs[i]));
    LogMessageCharDouble("+ i",imag(gs[i]));
    LogMessageCharInt(", Nocc[i]=",Nocc[i]);
    LogMessageChar("\n");
#endif // TEST_APPLYSZQ
       // *** TODO: This works only with spin flip,
       // or with an identity symmetry as 1st symmetry. Too specific solution.
       // FIX THIS !!! ***

    if (Nocc[i] != 0) //check if gsstate is compatible with current q
    { 
       
      state = 1;

#ifdef TEST_APPLYSZQ
      LogMessageChar("\n");
      for (int i = 0; i < Ndimensions; i++){
      LogMessageChar3Vector("Nunitcells, TransIds[i], q[TransIds[i]]: ",
                        Nsymvalue[TransIds[i]], TransIds[i], q[TransIds[i]]);
      }
#endif //TEST_APPLYSZQ

      //Loop over spins in u.c.
      for (int jpp = 0; jpp<Nspins_in_uc; jpp++){
        new_state = state;
        spin_pos_res = zero;

#ifdef TEST_APPLYSZQ
        LogMessageCharInt("\nChecking at position (bitmap) ", state);
#endif //TEST_APPLYSZQ
       
        //Loop over unit cells
        for (double x=0; x<Nsymvalue[TransIds[X]]; x++){
          for (double y=0; y<Nsymvalue[TransIds[Y]]; y++){
            for (double z=0; z<Nsymvalue[TransIds[Z]]; z++){
              phase = (
                    diffQ[TransIds[X]]*x/Nsymvalue[TransIds[X]]+
                    //y is always 0 if Ndim<2
                    diffQ[TransIds[Y]]*y/Nsymvalue[TransIds[Y]]+ 
                    //z is always 0 if Ndim<3
                    diffQ[TransIds[Z]]*z/Nsymvalue[TransIds[Z]]);
                                                    

#ifdef TEST_APPLYSZQ
              LogMessageChar3Vector("\n\t(x,y,z):", x,y,z);
              LogMessageCharDouble("\n\tphase: ",phase);
#endif //TEST_APPLYSZQ
       
              if (new_state&gsstate){ //Add or subtract depending on Sz
                spin_pos_res += exp(I*2.0*PI*phase);

#ifdef TEST_APPLYSZQ
                LogMessageChar("\nSpin UP");
#endif //TEST_APPLYSZQ

              }
              else{
                spin_pos_res -= exp(I*2.0*PI*phase);

#ifdef TEST_APPLYSZQ
                LogMessageChar("\nSpin DOWN");
#endif //TEST_APPLYSZQ
       
              }


#ifdef TEST_APPLYSZQ
              LogMessageCharKomplex("\nspin_pos_res:",spin_pos_res);
              LogMessageChar("\n");
#endif //TEST_APPLYSZQ

              if (Ndimensions == 3){
                new_state = SymOp(TransIds[Z], new_state);

#ifdef TEST_APPLYSZQ
                LogMessageCharInt("Going to (z)", new_state);
#endif //TEST_APPLYSZQ

              }
            }
            if (Ndimensions >= 2){
              new_state = SymOp(TransIds[Y], new_state);

#ifdef TEST_APPLYSZQ
              LogMessageCharInt("Going to (y)", new_state);
#endif //TEST_APPLYSZQ

            }
          }
          if (Ndimensions >= 1){
            new_state = SymOp(TransIds[X], new_state);

#ifdef TEST_APPLYSZQ
            LogMessageCharInt("Going to (x)", new_state);
#endif //TEST_APPLYSZQ
          }
        }

        double pos_phase = 0;
        for (int i = 0; i < Ndimensions; i++){ //Add position dependent phase
                                               //to all u.c.
          pos_phase += 
              diffQ[TransIds[i]]*spin_positions[jpp][i]/Nsymvalue[TransIds[i]];
        }
#ifdef TEST_APPLYSZQ
        LogMessageCharDouble("pos_phase:",pos_phase);
#endif //TEST_APPLYSZQ
        
        spin_pos_res *= exp(I*2.0*PI*pos_phase);

        res += spin_pos_res;

        state = state << 1; //Goto next spin in u.c.
      }

        res /= (2*sqroot[Nspins]);
#ifdef TEST_APPLYSZQ
      LogMessageCharDouble("\nAfter Transloop res =",real(res));
      LogMessageCharDouble("+ i",imag(res));
#endif // TEST_APPLYSZQ

      szxygs[i] = gs[i]*res;
    }
    else //Nocc == 0
    {
#ifdef TEST_APPLYSZQ
      LogMessageChar("State is forbidden!\n");
#endif
      szxygs[i] = zero;
    }

#ifdef TEST_APPLYSZQ
    //LogMessageCharInt ("ApplySzq output: m =",twom/2);
    LogMessageCharInt(", unique ",i);
    LogMessageCharInt("\ncorresponding to state ", unique[i]);
    LogMessageCharDouble("with weight: ",real(szxygs[i]));
    LogMessageCharDouble(" + i",imag(szxygs[i]));
    LogMessageChar("\n");
#endif
  } 

  return;
}

#ifndef M_SYM
/* ApplySmp applies the S+(q) or S-(q) operator (to the ground state vector).
   Last Change: Asbjørn 2025.02.6
   The SMP parameter determines which operator to apply
   SPMcross=0 :  s_q^+ |gs>
   SPMcross=1 :  s_q^- |gs>
   ApplySmp can be used to either get s^xx and s^yy or s^+- and s^-+.
   *resultvec is either spgs or smgs.
   */
void ApplySmp(long long *q, long long SPMcross, komplex *resultvec)
{
  long long diffQ[NSYM];
  int Tl[NSYM];
  unsigned long long gsstate, state, new_state,smp_state,l,u;
  long long ispossible;
  komplex res, phase_pp;
  komplex norm;

#ifdef TEST_APPLYSMP
  LogMessageCharInt("\nSPMcross =",SPMcross);
  LogMessageCharInt(", ApplySMP q =(",q[0]);
  LogMessageCharInt(",",q[1]);
  LogMessageCharInt(") - q_gs =(",q_gs[0]);
  LogMessageCharInt(",",q_gs[1]);
  LogMessageChar(")\n");
#endif

  for (long long sym=0;sym<Nsym;sym++) 
  {
    diffQ[sym]=q[sym]-q_gs[sym];
    if (diffQ[sym]<0) 
    {
      diffQ[sym]+=Nsymvalue[sym];
    }
    if (symlist[sym]==SPIN_FLIP && diffQ[sym]==0)
    {
      for(int i=0;i<Nunique;i++)
      {
        resultvec[i]=zero;
        return;
      }
    }
#ifdef TEST_APPLYSMP
    LogMessageCharInt("ApplySmp: sym=",sym);
    LogMessageCharInt(", diffQ=",diffQ[sym]);
    LogMessageCharInt(", q=",q[sym]);
    LogMessageCharInt(", q_gs=",q_gs[sym]);
    LogMessageCharInt(", Nsym=",Nsym);
    LogMessageCharInt(", symlist=",symlist[sym]);
    LogMessageChar("\n");
#endif // TEST_APPLYSMP
  } 


  for(int i=0;i<Nunique;i++) /* Run through unique */
  {    
    gsstate = unique[i];
    norm = gs[i]/(sqroot[Nspins] * sqroot[Nocc_0[i]]); 
                                                   
#ifdef TEST_APPLYSMP
    LogMessageCharInt("\n\ngsstate:",gsstate);
    LogMessageCharInt(", Nocc[i] =",Nocc[i]);  
    LogMessageCharInt(", Nocc_0[i] =",Nocc_0[i]);  
    LogMessageChar("\n");
#endif 


    /* Run through all translations of state */
    state = ((unsigned long long) 1); //bitmap for transloop

    for (int jpp=0; jpp<Nspins_in_uc; jpp++){
#ifdef TEST_APPLYSMP
      LogMessageCharInt("\nSpinmask = ",state);
#endif
        double pos_phase = 0;
        for (int i = 0; i < Ndimensions; i++){ //Add position dependent phase
                                               //to all u.c.
          pos_phase += 
              diffQ[TransIds[i]]*spin_positions[jpp][i]/Nsymvalue[TransIds[i]];
        }
#ifdef TEST_APPLYSMP
        LogMessageCharDouble(", pos_phase", pos_phase);
        LogMessageChar("\n");
#endif
        phase_pp = exp(I*2.0*PI*pos_phase);

        //Loop over all unit cells
        new_state = state;
        //Note that TransIds[Y/Z] = 0 unless defined. This then needs to be
        //either an identity or spinflip (Nsymvalue == 1).
        for (double x=0; x<Nsymvalue[TransIds[X]]; x++){
          for (double y=0; y<Nsymvalue[TransIds[Y]]; y++){
            for (double z=0; z<Nsymvalue[TransIds[Z]]; z++){
#ifdef TEST_APPLYSMP
                LogMessageCharInt("\nnew_state:",new_state);
            
                LogMessageCharInt("; Raise: ",!(gsstate & new_state));
                LogMessageCharInt(" Lower ->",(gsstate & new_state) != 0 );
#endif 

                //Is it possible to raise/lower spin jpp in unitcell xyz?
                if (SPMcross == 0) //raise
                {
                  ispossible = !(gsstate & new_state);
                }
                else //lower
                {
                  ispossible = (gsstate & new_state) != 0;
                }
            
                if( ispossible )
                { 
                  if (SPMcross == 0) //raise
                  {
                    smp_state=(gsstate | new_state);
                  }
                  else //lower
                  {
                    smp_state = (gsstate&~(new_state));
                  }
                
              u = FindUnique(smp_state,Tl); // Unique after updown operation;
                                            // Tl is the translation needed
                                            // from smp_state to unique
        
              l = LookUpU(u);  // Find position in table
        #ifdef TEST_APPLYSMP
              LogMessageCharInt("Changing to unique ",u);
              LogMessageCharInt("Tl: (",Tl[0]);
              for (int i = 1; i<Nsym; i++){
                LogMessageCharInt(", ",Tl[i]);
              }
              LogMessageChar(")");
              LogMessageCharInt(", Nocc[l]=",Nocc[l]);
              LogMessageCharInt(", Nocc_0[i]=",Nocc_0[i]);
              LogMessageChar("\n");
        #endif
              //Does this new unique exist in the current q-subspace?
              if (Nocc[l] != 0) {
                komplex tmp = phase_pp*norm*sqroot[Nocc[l]];

#ifdef TEST_APPLYSMP
                LogMessageCharKomplex("Partial result: ", tmp);
                LogMessageChar("\n");
#endif
                double dummy_phase = 
                    diffQ[TransIds[X]]*x/Nsymvalue[TransIds[X]]+
                    //y is always 0 if Ndim<2
                    diffQ[TransIds[Y]]*y/Nsymvalue[TransIds[Y]]+ 
                    //z is always 0 if Ndim<3
                    diffQ[TransIds[Z]]*z/Nsymvalue[TransIds[Z]];

#ifdef TEST_APPLYSMP
                LogMessageCharDouble("Phase from unit cell: ",dummy_phase);
                LogMessageChar("\n");
#endif
                //Calculate phase for unit cell at (x,y,z)
                tmp *= exp(2.0*I*PI*(dummy_phase));



                //Calculate phase component for raising/lowering this spin
                dummy_phase = 0;
                for (int i = 0; i<Nsym; i++){
                    dummy_phase += 1.0 * q[i]*Tl[i]/Nsymvalue[i];
                }
#ifdef TEST_APPLYSMP
                LogMessageCharDouble("Translation phase: ",dummy_phase);
                LogMessageChar("\n");
#endif
                tmp *= exp(2.0*I*PI*dummy_phase);

#ifdef TEST_APPLYSMP
                LogMessageCharKomplex("Contribution from this operation: ",
                    tmp/gs[i]);
                LogMessageChar("\n");
#endif

                resultvec[l] += tmp;
                
              }


          //End of this unit cell. Whereto next?
              if (Ndimensions == 3){
                new_state = SymOp(TransIds[Z], new_state);

#ifdef TEST_APPLYSMP
                LogMessageCharInt("Going to (z)", new_state);
#endif //TEST_APPLYSMP

              }
            }
            if (Ndimensions >= 2){
              new_state = SymOp(TransIds[Y], new_state);

#ifdef TEST_APPLYSMP
              LogMessageCharInt("Going to (y)", new_state);
#endif //TEST_APPLYSMP

            }
          }
          if (Ndimensions >= 1){
            new_state = SymOp(TransIds[X], new_state);

#ifdef TEST_APPLYSMP
            LogMessageCharInt("Going to (x)", new_state);
#endif //TEST_APPLYSMP
          }
        }
    }
        state = state << 1; //Goto next spin in u.c.
    }//for spin jpp in unit cell
    }//for Nunique

return;
}
#endif //NOT M_SYM

#ifdef NEVER //Msym old
/* ApplySmpMsym applies the S-S+(q) operator (to the ground state vector).
   Last Change: Kim 06.09.94 
   The SMP parameter determines which cross section to calculate:
   SPM=0 : S^+- = sum_e |<e| s_q^- |gs>|^2 , or
   SPM=1 : S^-+ = sum_e |<e| s_q^+ |gs>|^2
   */
void ApplySmp(long long *q, long long SPMcross)
{
  unsigned long long i,l,j;
  int T[Nsym];
  unsigned long long gsstate,gsstate0,mask,new_state,u,state,smp_state;
  long long phase;
  double norm;
  long long possible;

#ifdef TEST_APPLYSMP
  LogMessageCharInt("ApplySMP q =(",q[0]);
  LogMessageCharInt(",",q[1]);
  LogMessageCharInt(") - q_gs =(",q_gs[0]);
  LogMessageCharInt(",",q_gs[1]);
  LogMessageChar(")\n");
#endif


  for(i=0;i<Nunique;i++) /* Run through unique */
  {    
    gsstate0 = unique[i];

    /* Check if unique is component of ground state. */
    if(Nocc[i] != 0) //check if gsstate is compatible with current q
    {

      /* Run through possible spin-flips */
      for(j=0;j<Nspins;j++) 
      {
        mask = ((unsigned long long) 1)<<j;
#ifdef TEST_APPLYSMP
        LogMessageCharInt("\n\ngsstate:",gsstate);
        LogMessageCharInt(", mask:", mask);
        LogMessageCharInt("; Raise: ->",!(gsstate&mask));
        //LogMessageCharInt(" Lower ->",gsstate&mask);
#endif 
        /* Check if raising operation is possible */
        if (SPMcross == int(SMP)) //raise
        {
          possible=!(gsstate & mask);
        }
        else
        {
          possible = (gsstate & mask);
        }

        if( possible )
        { 
          if (SPMcross == int(SMP)) //raise
          {
            smp_state=(gsstate | mask);
          }
          else
          {
            smp_state = (gsstate&~(mask));
          }


#ifdef TEST_APPLYSMP
          LogMessageCharInt("\n j=",j);
          LogMessageCharInt("smp_state=",smp_state);
#endif

          u = FindUnique(smp_state,T); // Unique after updown operation
          l = LookUpU(u);  // Find position in table 
          LogMessageCharInt(", unique ",u);
          LogMessageCharInt(", Nocc[l]=",Nocc[l]);
          LogMessageCharInt(", Nocc[i]=",Nocc[i]);
          LogMessageChar("\n");
          if (Nocc[l] != 0)
          {
            /* Determine phase shift and norm*/
            norm = sqroot[Nocc[l]]/sqroot[Nocc[i]]; //from definition of unique
            norm /= sqroot[Nspins];
            //norm /=Nspins;
            phase = (q[1]-q_gs[1])*(j);
            if (phase < 0)
            {
              phase = Nspins - phase;
            }
#ifdef TEST_APPLYSMP
            LogMessageCharDouble("Norm =",norm);
            LogMessageCharDouble(", phase =",phase);
            LogMessageChar("\n");
#endif 
            phase = phase %Nspins;
            LogMessageCharDouble("Before: szxygs[l] =",real(szxygs[l]));
            LogMessageCharDouble("+i",imag(szxygs[l]));  
            double res_r=norm*(real(gs[i])*cosine[phase]-imag(gs[i])*sine[phase])+real(szxygs[l]);
            double res_i=norm*(real(gs[i])*sine[phase]+imag(gs[i])*cosine[phase])+imag(szxygs[l]);
            LogMessageCharDouble("\n Complex check: real part",res_r);
            LogMessageCharDouble(", imag: ",res_i);
            /* Add element to new state */
            szxygs[l] = szxygs[l]+norm*(cosine[phase] + I*sine[phase])*gs[i];  //Note: cosine[k] = cos(2*PI*k/Nspins); 
#ifdef TEST_APPLYSMP
            LogMessageCharDouble("\nAfter :cosine[phase] =",cosine[phase]);
            LogMessageCharDouble(", sine[phase] =",sine[phase]);
            LogMessageCharDouble(", gs[i] =",real(gs[i]));
            LogMessageCharDouble("+i",imag(gs[i]));
            LogMessageCharDouble("\nszxygs[l] =",real(szxygs[l]));
            LogMessageCharDouble("+i",imag(szxygs[l]));
#endif
          } // Nocc 
          else {szxygs[l]=zero;} //state is forbidden!
        }     //if states
      }      // j loop 
    }       // Nocc 
  }         // Unique loop 
  return;
}
#endif //NOT M_SYM

#ifdef M_SYM
/* ApplySmpMsym applies the S-S+(q) operator (to the ground state vector).
   Last Change: Kim 06.09.94 
   The which_q parameter determines if it is the S-(q)S+ operator 
   (which_q=0) or the S-S+(q) operator (which_q=1). */
void ApplySmpMsym(long long *q, long long which_q)
{
  unsigned long long i,l,j,k;
  int T[Nsym];
  unsigned long long gsstate,downup,updown,u,state,smp_state;
  long long phase,n_flip,u_cycle,new_cycle;
  double norm;
  long long u_occ;

#ifdef TEST_APPLYSMP
  LogMessageCharInt("ApplySMP q =(",q[0]);
  LogMessageCharInt(",",q[1]);
  LogMessageChar(")\n");
#endif


  for(i=0;i<Nunique;i++) /* Run through unique */
  {    
    gsstate = unique[i];
    u_occ = Nocc[i];

    /* Check if unique is component of ground state. */
    if(Nocc[i] != 0) 
    {
      n_flip=0;
      /* Run through possible spin-flips */
      for(k=0;k<Nspins;k++) {
        for(j=0;j<Nspins;j++) {

          downup = ((unsigned long long) 1)<<k;
          updown = ((unsigned long long) 1)<<j;
#ifdef TEST_APPLYSMP
          LogMessageCharInt("\ngsstate:",gsstate);
          LogMessageCharInt(", downup:", downup);
          LogMessageCharInt(", updown:",updown);
          LogMessageCharInt(" - downup gsstate ->",!(gsstate&downup));
          LogMessageCharInt(" updown gsstate ->",gsstate&updown);
#endif 
          /* Check if up-down operation is possible, including operation on the same spin */
          if( ( !(gsstate & downup)) && (( gsstate & updown) || (k==j)) )
          { 
            n_flip++;
            if (k==j)
              smp_state = gsstate;
            else
              smp_state = (gsstate | downup)&(~updown);

#ifdef TEST_APPLYSMP
            LogMessageCharInt("\nk=",k);
            LogMessageCharInt(" j=",j);
            LogMessageCharInt("smp_state=",smp_state);
#endif

            u = FindUnique(smp_state,T); // Unique after updown operation
            l = LookUpU(u);  // Find position in table 
#ifdef TEST_APPLYSMP
            LogMessageCharInt("SMP: Nocc[l]=",Nocc[l]);
            LogMessageCharInt(", T=",T[1]);
            LogMessageChar("\n");
#endif
            if (Nocc[l] != 0)
            {
              norm = sqroot[Nocc[l]]/sqroot[u_occ];

              //WARNING: THESE PHASES ARE EXPLICITLY FOR ANTIFERROMAGNETS ONLY, BEWARE
              // also, they do not quite work yet.
              /* Determine phase shift and norm*/
              if (which_q==1) {
                norm /= sqroot[2*(twom/2)*Nspins];  /* Sminus operator effect */
                phase = (2*Nspins*Nspins - k*q[1] + T[1]*(q[1]+q_gs[1]*Nspins/2) )%Nspins;
                if (phase > 0 && q_gs[1] == 1) {//skal de her vaere her??
                  phase = Nspins - phase;  // PATCH for complex conjugation 
                }
              }
              else if (which_q==0){
                norm /= sqroot[2*(twom/2+1)*Nspins];  /* Splus operator effect */
                phase = (2*Nspins*Nspins - j*q[1] + T[1]*(q[1]-q_gs[1]*Nspins/2) )%Nspins;
                if (phase > 0 && q_gs[1] == 1){
                  phase = Nspins - phase;  // PATCH for complex conjugation 
                }
              }

#ifdef TEST_APPLYSMP
              LogMessageCharDouble("Norm =",norm);
              LogMessageCharDouble(", phase =",phase);
              LogMessageChar("\n");
#endif                  
              /* Add element to new state */
              szxygs[l] += norm*(cosine[phase] - I*sine[phase])*gs[i];

            } // Nocc 
            else {szxygs[l]=zero;}
          }   // Interchangable 
        }     // Double k,j loop 
      }
    }       // Nocc 
  }         // Unique loop 


  return;
}
#endif //M_SYM

#ifdef NEVER
#ifdef MATRIX
// CrossMatrix calculates now only S^zz(q) on the whole set of eigenstates
// WARNING: all symmetries are considered to be spatial periodic translations !!
// Written by Kim, 14.07.00
void CrossMatrix(long long symvalue[NSYM])
{
  long long i,j,q[NSYM];
  komplex res,basis_vector[NUNIQUE], szq_vector[NUNIQUE];

  QLOOP_BEGIN  //* These are the Q's in S(Q), perhaps do this differently? 
    for (i=0; i<Nuniq_k; basis_vector[i++]=1);
  //    ApplySzq(basis_vector,symvalue,q); // basis_vector is the trace of a diagonal matrix 
  for (i=0; i<Nuniq_k; i++)
  {
    res=0;
    for (j=0; j<Nuniq_k; j++)
      //        res+=basis_vector[j]*eigenstates[i][j]; // Multiplying with a diagonal matrix 
      //      sqz_vector=res*conj(res)
      ;
  }
  QLOOP_END

    return;
}
#endif /* MATRIX */

bool NonZero(unsigned long long state, long long *q) {
  // simple implementation of routine to determine if state is value in q-space q
  // NEVER used? - ABP 2025.02.21
  unsigned long long new_state;
  long long eksponent;
  long long sym, T[NSYM];

  LogMessageCharInt("\nIn NonZero:  state =",state);
  LogMessageCharInt(" q =(",q[0]);
  LogMessageCharInt(",",q[1]);
  LogMessageChar(")\n");

  TLOOP_BEGIN // goes through all symmetries defined, setting new_state and T[]
    if (new_state==state) { // if we got back to the unique
      eksponent =0;
      for (long long i=0;i<Nsym;i++) {
        eksponent+=q[i]*T[i]*Nspins/Nsymvalue[i];  // q_i*d_i*N/n_i

        LogMessageCharInt("i =",i);
        LogMessageCharInt("T[i] =",T[i]);
        LogMessageCharInt(", eksponent%Nspins =",eksponent%Nspins);
        LogMessageChar("\n");

      }
      if (eksponent%Nspins != 0) { // equiv. "if coefficient not 1"
        LogMessageChar(" - Non Zero false!\n");
        return false; // coefficient will add up to 0
      }
    }
  TLOOP_END
    LogMessageChar(" - Non Zero true!\n");
  return true; // all coefficients were equal to one.
}

#endif /*NEVER*/

double lengthofvector(komplex *v) {
  double length=0.0;
  for (long long i=0;i<Nunique;i++) {
    length+=((real(v[i])*real(v[i]))+(imag(v[i])*imag(v[i])));
  }
  return length;
}


#endif /* FIND_CROSS */
