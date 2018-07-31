/*----------------------------------------------------------------------------
 ADOL-C -- Automatic Differentiation by Overloading in C++
 File:     LUsolve_MT.cpp
 Revision: $Id$
 Contents: Serves as example and test for
             * Computation of the determinant of a matrix
               by LU-decomposition of the system matrix without pivoting 
             * application of tapedoc to observe taping of
               the new op_codes for the elementary operations

                     y += x1 * x2;
                     y -= x1 * x2;

             * application of par_jacobian driver
             * comparison of Jacobian obtained from jacobian and par_jacobian
             * handling several tapes while using par_jacobian

 Usage:
   see usage()

 First, the LU decomposition for the provided system sizes is traced. In a
 second step, the drivers jacobian and par_jacobian are called to obtain
 the Jacobian matrices. For each system size the matrices returned from
 jacobian and par_jacobian are compared.

 After doing so in ascending order for system sizes (calc_seq), the drivers
 are called in random order (calc_rand).


 Copyright (c) Andrea Walther, Andreas Griewank, Andreas Kowarz, 
               Hristo Mitev, Sebastian Schlenkrich, Jean Utke, Olaf Vogel,
               Martin Schroschk
  
 This file is part of ADOL-C. This software is provided as open source.
 Any use, reproduction, or distribution of the software constitutes 
 recipient's acceptance of the terms of the accompanying license file.
 
---------------------------------------------------------------------------*/

/****************************************************************************/
/*                                                                 INCLUDES */
#include "LU.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <string>


// At least some const correctness.
typedef const double* const* constMat;

// Global counter to ensure, that every Trace has a unique tag.
static int tagCntr = 0;

class Problem
{
public:
  ~Problem() {
    myfree1(args);
    myfree1(x);
  }
  uint size = 0;
  int tag = -1;
  uint depen = 0;
  uint indep = 0;
  double* args = NULL ;
  double* x = NULL;
};


int calc_seq(const std::vector<uint>& sizes);
int calc_rand(const std::vector<uint>& sizes);
int compute_and_trace(Problem& p);
int apply_drivers(Problem& p);
void mat_print(const std::string& name, const uint m, const uint n, constMat M);
int compare_mats(const uint m, const uint n, constMat jac1, const std::string& name1,
                 constMat jac2, const std::string& name2);
void usage()
{
  std::cout << "Usage: OMP_NUM_THREADS=N ./LUsolve_MT [SIZE1 [, SIZE2 [, SIZE3 ...]]] \n";
}

/****************************************************************************/
/*                                                             MAIN PROGRAM */
/*--------------------------------------------------------------------------*/
int main(int argc, char* argv []) {

    // Parse arguments / sizes
    std::vector<uint> sizes;
    if (2 <= argc) {
      for (int i = 1; i < argc; ++i) {
       int size = atoi(argv[i]);
       if (1 > size) {
         usage();
         return 1;
       }
       else
         sizes.push_back(size);
      }
    } else {
      usage();
      return 0;
    }

    // Since the tag numbering is identical to problem sizes, we remove duplicates.
    std::sort(sizes.begin(), sizes.end());
    sizes.erase(std::unique(sizes.begin(), sizes.end()), sizes.end());

    /*------------------------------------------------------------------------*/
    /* Info */
    std::cout << "LINEAR SYSTEM SOLVING by LU-DECOMPOSITION (ADOL-C Example)\n\n";

    std::cout << "=========================================================\n";
    std::cout << " Evaluation in sequential order \n";
    std::cout << "=========================================================\n";
    calc_seq(sizes);

    std::cout << "=========================================================\n";
    std::cout << " Evaluation in alternate order \n";
    std::cout << "=========================================================\n";
    calc_rand(sizes);

    return 0;
}

int calc_rand(const std::vector<uint>& sizes) {
  int ret = 0;
  std::vector<Problem> problems(sizes.size());
  std::vector<uint> shuffledSizes = sizes;
  std::random_shuffle(shuffledSizes.begin(), shuffledSizes.end());

  for (uint i = 0; i < shuffledSizes.size(); ++i) {
    std::cout << "=== System size is: " << shuffledSizes[i] << "\n";
    problems[i] = Problem{shuffledSizes[i], tagCntr++};
    compute_and_trace(problems[i]);
    apply_drivers(problems[i]);
    std::cout << "\n";
  }

  return ret;
}

int calc_seq(const std::vector<uint>& sizes) {
  int ret = 0;
  std::vector<Problem> problems(sizes.size());

  for (uint i = 0; i < sizes.size(); ++i) {
    std::cout << "=== System size is: " << sizes[i] << "\n";
    problems[i] = Problem{sizes[i], tagCntr++};
    compute_and_trace(problems[i]);
    apply_drivers(problems[i]);
    std::cout << "\n";
  }

  return ret;
}

int compute_and_trace(Problem& p) {
  int ret = 0;

  // Variables
  int size = p.size;
  if (0 >= size)
    return 1;

  p.indep = size*size+size;          // # of indeps
  p.depen = size;                    // # of deps

  // Passive variables
  double** A = myalloc2(size, size);
  double* a1 = myalloc1(size);
  double* a2 = myalloc1(size);
  double* b = myalloc1(size);
  p.x = myalloc1(size);
  adouble **AA, *AAp, *Abx;         // active variables
  p.args = myalloc1(p.indep);       // arguments


  /*------------------------------------------------------------------------*/
  /* Allocation and initialization of the system matrix */
  AA  = new adouble*[size];
  AAp = new adouble[size*size];
  for (int i = 0; i < size; ++i) {
    AA[i] = AAp;
    AAp += size;
  }

  Abx = new adouble[size];
  for (int i = 0; i < size; ++i) {
    a1[i] = i*0.25;
    a2[i] = i*0.33;
  }

  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; ++j)
      A[i][j] = a1[i]*a2[j];
    A[i][i] += i+1;
    b[i] = -i-1;
  }

  /*------------------------------------------------------------------------*/
  /* Taping the computation of the determinant */
  trace_on(p.tag);
  /* marking indeps */
  for (int i = 0; i < size; ++i)
    for (int j = 0; j < size; ++j)
      AA[i][j] <<= (p.args[i*size+j] = A[i][j]);
  for (int i = 0; i < size; ++i)
    Abx[i] <<= (p.args[size*size+i] = b[i]);
  /* LU-factorization and computation of solution */
  LUfact(size,AA);
  LUsolve(size,AA,Abx);
  /* marking deps */
  for (int i = 0; i< size; ++i)
    Abx[i] >>= p.x[i];
  trace_off();
  std::cout << "  x[0] (original) : " << std::scientific << p.x[0] << "\n";

  /*------------------------------------------------------------------------*/
  /* Recomputation  */
  function(p.tag, p.depen, p.indep, p.args, p.x);
  std::cout << "  x[0] (from tape): " << std::scientific << p.x[0] << "\n";

  myfree1(b);
  myfree1(a1);
  myfree1(a2);
  myfree2(A);
  delete[] Abx;
  delete[] AA;

  return ret;
}

int apply_drivers(Problem& p) {
  int ret = 0;

  if (0 >= p.size)
    return 1;

  /*------------------------------------------------------------------------*/
  /* Computation of Jacobian */
  double** jac = myalloc2(p.depen, p.indep);
  jacobian(p.tag, p.depen, p.indep, p.args, jac);
  if (6 > p.size)
    mat_print("Jacobian", p.depen, p.indep, jac);

  /*------------------------------------------------------------------------*/
  /* Parallel computation of Jacobian */
  double** parJ = myalloc2(p.depen, p.indep);
  par_jacobian(p.tag, p.depen, p.indep, p.args, parJ);
  if (6 > p.size)
    mat_print("Par Jacobian", p.depen, p.indep, parJ);

  /*------------------------------------------------------------------------*/
  /* Compare Jacobian and Parallel Jacobian*/
  compare_mats(p.depen, p.indep, jac, "jac", parJ, "parJac");

  /*------------------------------------------------------------------------*/
  /* Tape documentation */
  tape_doc(p.tag, p.depen, p.indep, p.args, p.x);

  /*------------------------------------------------------------------------*/
  /* Tape statistics */
  ulong tape_stats[STAT_SIZE];
  tapestats(p.tag, tape_stats);

  std::cout << "  Tape Statistics:\n";
  std::cout << "    independents            " << tape_stats[NUM_INDEPENDENTS]
            << "\n    dependents              " << tape_stats[NUM_DEPENDENTS]
            << "\n    operations              " << tape_stats[NUM_OPERATIONS]
            << "\n    operations buffer size  " << tape_stats[OP_BUFFER_SIZE]
            << "\n    locations buffer size   " << tape_stats[LOC_BUFFER_SIZE]
            << "\n    constants buffer size   " << tape_stats[VAL_BUFFER_SIZE]
            << "\n    maxlive                 " << tape_stats[NUM_MAX_LIVES]
            << "\n    valstack size           " << tape_stats[TAY_STACK_SIZE] << "\n\n";

  myfree2(parJ);
  myfree2(jac);

  /*------------------------------------------------------------------------*/
  /* That's it */
  return ret;
}

/******************************************************************************/
void mat_print(const std::string& name, const uint m, const uint n, constMat M)
{
  std::cout << "\n Print matrix " << name << " (" << m << "x" << n << "):\n";
  for(uint i = 0; i < m ; ++i) {
    std::cout << "  " << i << ": ";
    for(uint j = 0; j < n ; ++j)
      std::cout << std::setprecision(4) << std::fixed << M[i][j] << "  ";
    std::cout << "\n";
  }
  std::cout << "\n";
}

/******************************************************************************/
int compare_mats(const uint m, const uint n, constMat jac1, const std::string& name1,
                 constMat jac2, const std::string& name2)
{
  double eps = 1.E-10;
  double f;
  int ret = 0;

  std::cout << "\n  Compare results:\n";
  for (uint i = 0; i < m ; ++i) {
    for (uint j = 0; j < n ; ++j) {
      f = fabs(jac1[i][j] - jac2[i][j]);
      if (f > eps) {
        std::cout << "\tUnexpected value: expected[" << i << "][" << j << "] = "
            << jac1[i][j]
            << " vs result[" << i << "][" << j << "] = " << jac2[i][j] << "\n";
        ret = 1;
      }
    }
  }
  if (!ret)
    std::cout << "    " << name1 << " and " << name2 << " are identical within eps "
              << std::scientific << eps << ".\n";
  std::cout << "\n";

  return ret;
}
