/*
 * Copyright (C) 2016 Leo Fang <leofang@phy.duke.edu>
 *
 * This program is free software. It comes without any warranty,
 * to the extent permitted by applicable law. You can redistribute
 * it and/or modify it under the terms of the WTFPL, Version 2, as
 * published by Sam Hocevar. See the accompanying LICENSE file or
 * http://www.wtfpl.net/ for more details.
 */

#include "dynamics.h"
#include <math.h>

// this function computes the average of 4 points that form a square
// j and i are the array indices psi[j][i] of the upper right corner
double complex square_average(int j, int i, grid * simulation)
{
   if(i<1) //beyond the boundary x=-(Nx+nx+1)*Delta
   {
      fprintf(stderr, "Error in %s: beyond left boundary. Abort!\n", __func__);
      exit(EXIT_FAILURE);
   }

   if(i>simulation->Ntotal-1) //beyond the boundary x=Nx*Delta
   {
      fprintf(stderr, "Error in %s: beyond right boundary. Abort!\n", __func__);
      exit(EXIT_FAILURE);
   }

   if(j<1) //everything is zero below t=0
      return 0;

   double complex square = 0;

   for(int m=j-1; m<=j; m++)
   {
      for(int n=i-1; n<=i; n++)
         square += simulation->psi[m][n];
   }
   square/=4.;

   return square;
}


// this function computes the average of 2 points that form a bar
// j and i are the array indices psi[j][i] of the right end
double complex bar_average(int j, int i, grid * simulation)
{
   if(i<1) //beyond the boundary x=-(Nx+nx+1)*Delta
   {
      fprintf(stderr, "Error in %s: beyond left boundary. Abort!\n", __func__);
      exit(EXIT_FAILURE);
   }

   if(i>simulation->Ntotal-1) //beyond the boundary x=Nx*Delta
   {
      fprintf(stderr, "Error in %s: beyond right boundary. Abort!\n", __func__);
      exit(EXIT_FAILURE);
   }

   if(j<0) //everything is zero below t=0
      return 0;

   return (simulation->psi[j][i]+simulation->psi[j][i-1])/2.;
}


//this function computes chi(x1, x2, 0)
//update: arguments x1 & x2 now refer to the "unit-less" coordinates, so "true x1" = x1 * Delta and so on
double complex two_photon_input(double x1, double x2, grid * simulation)
{
   double complex chi = 0;
   switch(simulation->init_cond)
   {
      //two-photon plane waves
      case 1: { chi = cexp( I * simulation->k * (x1+x2) * simulation->Delta); } break;

      //two-photon exponentail wavepacket (init_cond=3)
      case 3: {
         if(simulation->identical_photons)
            chi = one_photon_exponential(x1, simulation->k, simulation->alpha, simulation) \
                  * one_photon_exponential(x2, simulation->k, simulation->alpha, simulation);
         else
         {
            chi = simulation->A / sqrt(2.) *
                  ( one_photon_exponential(x1, simulation->k1, simulation->alpha1, simulation) \
                    * one_photon_exponential(x2, simulation->k2, simulation->alpha2, simulation) \
                    + one_photon_exponential(x2, simulation->k1, simulation->alpha1, simulation) \
                    * one_photon_exponential(x1, simulation->k2, simulation->alpha2, simulation) );
         }
      } break;

      //TODO: add other different inputs here
      
      default: { exit(EXIT_FAILURE); }
   }

   return chi;
}


// this function computes the exponential wavepacket with a sharp wavefront at x=-a
// update: argument x now refer to the "unit-less" coordinates, so "true x" = x * Delta
double complex one_photon_exponential(double x, double k, double alpha, grid * simulation)
{
   if(x>simulation->minus_a_index)
      return 0;

   double a_g = alpha * simulation->Gamma; 
   
   return I*csqrt(a_g) * cexp( (I*k*x + 0.5*a_g*(x+0.5*simulation->nx)) * simulation->Delta );
}
