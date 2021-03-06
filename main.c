/*
 * Copyright (C) 2016 Leo Fang <leofang@phy.duke.edu>
 *
 * This program is free software. It comes without any warranty,
 * to the extent permitted by applicable law. You can redistribute
 * it and/or modify it under the terms of the WTFPL, Version 2, as
 * published by Sam Hocevar. See the accompanying LICENSE file or
 * http://www.wtfpl.net/ for more details.
 */

#include <math.h>
#include "grid.h"
#include "kv.h"
#include "dynamics.h"
#include "NM_measure.h"


int main(int argc, char **argv)
{
   if(argc != 2)
   {
      fprintf(stderr, "Usage: ./FDTD input_parameters\n");
      exit(EXIT_FAILURE);
   }
   
   printf("FDTD: solving 1+1D delay PDE\n");
   printf("This code is released under the WTFPL without any warranty.\n");
   printf("See LICENSE or http://www.wtfpl.net/ for more details.\n");
   printf("Copyright (C) 2016 Leo Fang\n\n");
   //printf("For the academic uses, citation to (ref) is strongly encouraged but not required.\n");
   
   printf("FDTD: preparing the grid...\n");
   grid * simulation = initialize_grid(argv[1]);
//   printf("\033[F\033[2KFDTD: preparing the grid...Done!\n");
   printf("FDTD: simulation starts...\n");// fflush(stdout);

   // W = (i*w0+Gamma/2)
   double complex W = simulation->w0*I+0.5*simulation->Gamma;

   //simulation starts
   for(int j=1; j<simulation->Ny; j++) //start from t=1*Delta
   {
       for(int i=simulation->nx+1; i<simulation->Ntotal; i++) //start from x=-Nx*Delta
       {
           //points (i) right next to the 1st light cone and in tile B1, 
           //and (ii) right next to the 2nd light cone 
           //should be strictly zero under any circumstances
           if( ((j < simulation->nx) && (i==j+simulation->minus_a_index+1))
               || (i==j+simulation->plus_a_index+1) )
               continue; //do nothing, as psi is already zero initailized

           //free propagation (decay included)
           simulation->psi[j][i] = (1./simulation->Delta-0.25*W)*simulation->psi[j-1][i-1]   \
                                   -0.25*W*(simulation->psi[j-1][i]+simulation->psi[j][i-1]);
           
           //delay term: psi(x-2a, t-2a)theta(t-2a)
           if(j>simulation->nx)
               simulation->psi[j][i] += 0.5*simulation->Gamma*square_average(j-simulation->nx, i-simulation->nx, simulation);
   
           //left light cone No.1: psi(-x-2a, t-x-a)theta(x+a)theta(t-x-a)
           if( (i>simulation->minus_a_index) && (j-i>=-simulation->minus_a_index) )
           { 
               double on_light_cone = (j-i == -simulation->minus_a_index?0.5:1.0);
               simulation->psi[j][i] -= 0.5*simulation->Gamma*bar_average(j-(i-simulation->origin_index)-simulation->nx/2, \
                                        2*simulation->origin_index-i-simulation->nx+1, simulation)*on_light_cone; 
           }
   
           //left light cone No.2: -psi(-x, t-x-a)theta(x+a)theta(t-x-a)
           if( (i>simulation->minus_a_index) && (j-i>=-simulation->minus_a_index) )
           { 
               double on_light_cone = (j-i == -simulation->minus_a_index?0.5:1.0);
               simulation->psi[j][i] += 0.5*simulation->Gamma*bar_average(j-(i-simulation->origin_index)-simulation->nx/2, \
                                        2*simulation->origin_index-i+1, simulation)*on_light_cone; 
           }
    
           //right light cone No.1: psi(2a-x, t-x+a)theta(x-a)theta(t-x+a)
           if( (i>simulation->plus_a_index) && (j-i>=-simulation->plus_a_index) )
           { 
               double on_light_cone = (j-i == -simulation->plus_a_index?0.5:1.0);
               simulation->psi[j][i] -= 0.5*simulation->Gamma*bar_average(j-(i-simulation->origin_index)+simulation->nx/2, \
                                        2*simulation->origin_index-i+simulation->nx+1, simulation)*on_light_cone; 
           }
    
           //right light cone No.2: -psi(-x, t-x+a)theta(x-a)theta(t-x+a)
           if( (i>simulation->plus_a_index) && (j-i>=-simulation->plus_a_index) )
           { 
               double on_light_cone = (j-i == -simulation->plus_a_index?0.5:1.0);
               simulation->psi[j][i] += 0.5*simulation->Gamma*bar_average(j-(i-simulation->origin_index)+simulation->nx/2, \
                                        2*simulation->origin_index-i+1, simulation)*on_light_cone; 
           }
   
           //two-photon input: 2*( chi(x-t,-a-t, 0)-chi(x-t,a-t,0) )
           if( (simulation->init_cond == 1 || simulation->init_cond == 3) \
	       && j-i>=-simulation->minus_a_index ) //it's nonzero only when t-x-a>=0 
           { 
               double on_light_cone = (j-i == -simulation->minus_a_index?0.5:1.0);

               //shift +0.5 due to Taylor expansion at the center of square 
               simulation->psi[j][i] += sqrt(simulation->Gamma) * on_light_cone \
                                        * two_photon_input((i-simulation->origin_index)-j, -simulation->nx/2-j+0.5, simulation);

               if(j>simulation->nx)
               {
                   simulation->psi[j][i] -= sqrt(simulation->Gamma) * on_light_cone \
                                            * two_photon_input((i-simulation->origin_index)-j, simulation->nx/2-j+0.5, simulation);
               }
           }
   
           //prefactor
           simulation->psi[j][i] /= (1./simulation->Delta+0.25*W);
       }
   }
   //printf("Done!\n");

   printf("FDTD: writing results to files...\n");// fflush(stdout);
//   print_initial_condition(simulation);
//   print_boundary_condition(simulation);
//   printf("******************************************\n");
//   print_psi(simulation);
//   print_grid(simulation);
   if(simulation->save_psi)
   {
      save_psi(simulation, argv[1], creal);
      save_psi(simulation, argv[1], cimag);
      //save_psi(simulation, argv[1], cabs);
   }
   if(simulation->save_psi_square_integral) //for testing init_cond=3
      save_psi_square_integral(simulation, argv[1]); 
   if(simulation->save_psi_binary)
      save_psi_binary(simulation, argv[1]);
   if(simulation->save_chi)
      save_chi(simulation, argv[1], cabs);
   if(simulation->measure_NM)
   {
      printf("FDTD: calculating lambda and mu for NM measures...\n"); fflush(stdout);
      calculate_NM_measure(simulation, argv[1]);
      save_e0(simulation, argv[1], creal);
      save_e0(simulation, argv[1], cimag);
      save_e1(simulation, argv[1], creal);
      save_e1(simulation, argv[1], cimag);
   }

   //printf("Done!\n");

   free_grid(simulation);

   return EXIT_SUCCESS;
}
