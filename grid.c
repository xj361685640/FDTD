/*
 * Copyright (C) 2016 Leo Fang <leofang@phy.duke.edu>
 *
 * This program is free software. It comes without any warranty,
 * to the extent permitted by applicable law. You can redistribute
 * it and/or modify it under the terms of the WTFPL, Version 2, as
 * published by Sam Hocevar. See the accompanying LICENSE file or
 * http://www.wtfpl.net/ for more details.
 */

#include <stdlib.h>
#include "kv.h"
#include "grid.h"
#include "special_function.h"
#include "dynamics.h"
#include <string.h>

// This function returns the solution psi[j][i] in x<-a subject to incident plane wave
complex plane_wave_BC(int j, int i, grid * simulation)
{
    double x = (i-simulation->origin_index)*simulation->Delta; //check!!!
    double t = j*simulation->Delta;
    double td = simulation->nx*simulation->Delta;
    double k = simulation->k;
    double w0 = simulation->w0;
    double Gamma = simulation->Gamma;
    complex p = k - w0 + 0.5*I*Gamma;

    complex e_t = I*sqrt(0.5*Gamma)*cexp(-0.5*I*k*td)*(cexp(-I*k*t)-cexp(-I*w0*t-0.5*Gamma*t))/p;
    complex sum = 0;

    for(int n=1; n<=(j/simulation->nx); n++)
    {
        complex temp = cpow(0.5*Gamma, n-0.5) * \
                       ( cexp( n*log(t-n*td) + n*(I*w0*td+0.5*Gamma*td)-I*w0*t-0.5*Gamma*t-lgamma(n+1) ) \
                       + (k-w0)*incomplete_gamma(n+1, -I*p*(t-n*td))*cexp( n*clog(I)+I*n*k*td-I*k*t-(n+1)*clog(p) ) );

	// based on my observation, the wavefunction should converge very fast, 
	// so one can just cut the summation off if the precision is reached.
	// this also helps prevent some overflow issue a bit.
        if( cabs(temp) < DBL_EPSILON*cabs(sum) || isnan(temp) )
           break;
        else
	   sum += temp;
    }
    e_t -= cexp(-0.5*I*k*td)*sum;
    e_t *= sqrt(2.)*cexp(I*k*(x-t)); // psi(x,t) = sqrt(2)e^{ik(x-t)}*e(t)

    if(!isnan(e_t))
       return e_t;
    else
    {
       fprintf(stderr, "%s: NaN is produced (at j=%i and i=%i). Abort!\n", __func__, j, i);
       abort();
    }
}


// This function returns the solution psi[j][i] in x<-a subject to single-photon exponential wavepacket //TODO
complex exponential_BC(int j, int i, grid * simulation)
{
    double x = (i-simulation->origin_index)*simulation->Delta;
    double t = j*simulation->Delta;
    double td = simulation->nx*simulation->Delta;
    double k = simulation->k;
    double w0 = simulation->w0;
    double Gamma = simulation->Gamma;
    complex W = I*w0 + 0.5*Gamma;

    complex sum = 0;
    for(int n=1; n<=(j/simulation->nx); n++)
    {
        complex temp = exp(-lgamma(n+1)) * cpow(0.5*Gamma*cexp(W*td)*(t-n*td), n);

	// based on my observation, the wavefunction should converge very fast, 
	// so one can just cut the summation off if the precision is reached.
	// this also helps prevent some overflow issue a bit.
        if( cabs(temp) < DBL_EPSILON*cabs(sum) || isnan(temp) )
           break;
        else
	   sum += temp;
    }
    complex e_t = cexp(-W*t)*(1.0+sum);
    e_t *= one_photon_exponential(i-j, simulation); // psi(x,t) = psi(x-t, 0) * e(t)

    if(!isnan(e_t))
       return e_t;
    else
    {
       fprintf(stderr, "%s: NaN is produced (at j=%i and i=%i). Abort!\n", __func__, j, i);
       abort();
    }
}


void initial_condition(grid * simulation)
{// the initial condition is given in-between x/Delta = [-Nx, Nx] for simplicity

    simulation->psit0 = calloc(2*simulation->Nx+1, sizeof(*simulation->psit0));
    if(!simulation->psit0)
    { 
        perror("initial_condition: cannot allocate memory. Abort!\n");
        exit(EXIT_FAILURE);
    }
    simulation->psit0_size = 2*simulation->Nx+1;

    //for nonzero initial conditions
    if(simulation->init_cond == 2) //single-photon exponential wavepacket
    {
       for(int i=0; i<simulation->psit0_size; i++)
           simulation->psit0[i] = one_photon_exponential(i+simulation->nx+1, simulation);
    }
    //TODO: add other I.C. here
}


void boundary_condition(grid * simulation)
{// the boundary conditions is given for first nx+1 columns with 
 // x/Delta=[-(Nx+nx+1),-(Nx+1)] due to the delay term

    simulation->psix0 = malloc( simulation->Ny*sizeof(*simulation->psix0) );
    if(!simulation->psix0)
    { 
        perror("boundary_condition: cannot allocate memory. Abort!\n");
        exit(EXIT_FAILURE);
    }
    simulation->psix0_x_size = simulation->nx+1;
    simulation->psix0_y_size = 0;

    int progress = 0;
    for(int j=0; j<simulation->Ny; j++)
    {
        simulation->psix0[j] = calloc(simulation->nx+1, sizeof(*simulation->psix0[j])); 
        if(!simulation->psix0[j])
        { 
            fprintf(stderr, "%s: cannot allocate memory at t=%d*Delta. Abort!\n", __func__, j);
            exit(EXIT_FAILURE);
        }
        simulation->psix0_y_size++;
    }

    for(int j=0; j<simulation->psix0_y_size; j++)
    {
        switch(simulation->init_cond)
	{
	   case 1: { //two-photon plane wave
                 for(int i=0; i<simulation->psix0_x_size; i++)
                     simulation->psix0[j][i] = plane_wave_BC(j, i, simulation);
	      }
	      break;
	   case 2: { //single-photon exponential wavepacket
                 for(int i=0; i<simulation->psix0_x_size; i++)
                     simulation->psix0[j][i] = exponential_BC(j, i, simulation);
	      }
	      break;
	   default: { //bad input
              fprintf(stderr, "%s: invalid option. Abort!\n", __func__);
              exit(EXIT_FAILURE);
	      } 
        }

        if(j%(simulation->Ny/10)==0)
        {
            printf("%s: %i%% prepared...\r", __func__, progress*10); fflush(stdout);
            progress++;
        }
    }

    //wash out the status report
    printf("                                                                           \r"); fflush(stdout);
}


void initialize_psi(grid * simulation)
{
    simulation->psi = malloc( simulation->Ny*sizeof(*simulation->psi) );
    if(!simulation->psi)
    { 
        perror("initialize_psi: cannot allocate memory. Abort!\n");
        exit(EXIT_FAILURE);
    }
    simulation->psi_x_size = simulation->Ntotal;
    simulation->psi_y_size = 0;
    for(int j=0; j<simulation->Ny; j++)
    {
        simulation->psi[j] = calloc( simulation->Ntotal, sizeof(*simulation->psi[j]) );
        if(!simulation->psi[j])
        { 
            fprintf(stderr, "%s: cannot allocate memory at t=%d*Delta. Abort!\n", __func__, j);
            exit(EXIT_FAILURE);
        }
        simulation->psi_y_size++;

        // take boundary conditions
        for(int i=0; i<simulation->psix0_x_size; i++)
           simulation->psi[j][i] = simulation->psix0[j][i];
    }
 
    // take the initial condition
    for(int i=0; i<simulation->psit0_size; i++)
       simulation->psi[0][i+simulation->psix0_x_size] = simulation->psit0[i];
}


void sanity_check(grid * simulation)
{
    //nx must be multiple of 2
    if(simulation->nx % 2) 
    {
        fprintf(stderr, "%s: nx must be an integer multiple of 2. Abort!\n", __func__);
        exit(EXIT_FAILURE);
    }   

    //nx<=2Nx
    if(simulation->nx > 2*simulation->Nx)
    {
        fprintf(stderr, "%s: nx must be smaller than, or at most equal to, twice of Nx (nx<=2Nx). Abort!\n", __func__);
        exit(EXIT_FAILURE);
    } 

    //Nyquist limit
    if(simulation->k >= M_PI/simulation->Delta || simulation->w0 >= M_PI/simulation->Delta)
    {
        fprintf(stderr, "%s: k or w0 must be smaller than pi/Delta in order not to reach the Nyquist limit. Abort!\n", __func__);
        exit(EXIT_FAILURE);
    }

    //poka-yoke: meaningless if one performs the computation without saving any result
    if(!simulation->save_chi && !simulation->save_psi)
    {
        fprintf(stderr, "%s: either save_chi or save_psi has to be 1. Abort!\n", __func__);
        exit(EXIT_FAILURE);
    }

    //poka-yoke: meaningless if the initial condition is not correctly given
    //currently the allowed values are:
    //1 (two-photon plane wave)
    //2 (single-photon exponential wavepacket)
    if(simulation->init_cond < 1 || simulation->init_cond > 2)
    {
        fprintf(stderr, "%s: init_cond has to be 1 or 2. Abort!\n", __func__);
        exit(EXIT_FAILURE);
    }
    else if(simulation->init_cond == 2 && !lookupValue(simulation->parameters_key_value_pair, "alpha"))
    {//want to use exponential wavepacket but forget to set alpha's value
        fprintf(stderr, "%s: alpha is not given. Abort!\n", __func__);
        exit(EXIT_FAILURE);
    }
}


void free_initial_boundary_conditions(grid * simulation)
{//free psit0 and psix0 to save memory
    free(simulation->psit0);
    for(int j=0; j<simulation->Ny; j++)
       free(simulation->psix0[j]);
    free(simulation->psix0);
  
    //reset
    simulation->psix0 = NULL;
    simulation->psit0 = NULL;
    simulation->psix0_x_size = 0;
    simulation->psix0_y_size = 0;
    simulation->psit0_size = 0;
}


void free_grid(grid * simulation)
{
    freeKVs(simulation->parameters_key_value_pair);

    //free psit0 and psix0
    free_initial_boundary_conditions(simulation);

    for(int j=0; j<simulation->Ny; j++)
       free(simulation->psi[j]);
    free(simulation->psi);

    free(simulation);
}


grid * initialize_grid(const char * filename)
{
   grid * FDTDsimulation = malloc(sizeof(*FDTDsimulation));
   if(!FDTDsimulation)
   {
      perror("initialize_grid: could not allocate memory. Abort!\n");
      exit(EXIT_FAILURE);
   }
   
   //read from input
   FDTDsimulation->parameters_key_value_pair = readKVs(filename);

   //initialize from the input parameters
   FDTDsimulation->nx            = atoi(lookupValue(FDTDsimulation->parameters_key_value_pair, "nx"));
   FDTDsimulation->Nx            = atoi(lookupValue(FDTDsimulation->parameters_key_value_pair, "Nx"));
   FDTDsimulation->Ntotal        = 2 * FDTDsimulation->Nx + FDTDsimulation->nx + 2;
   FDTDsimulation->Ny            = atoi(lookupValue(FDTDsimulation->parameters_key_value_pair, "Ny"));
   FDTDsimulation->Delta         = strtod(lookupValue(FDTDsimulation->parameters_key_value_pair, "Delta"), NULL);
   FDTDsimulation->k             = strtod(lookupValue(FDTDsimulation->parameters_key_value_pair, "k"), NULL);
   FDTDsimulation->w0            = strtod(lookupValue(FDTDsimulation->parameters_key_value_pair, "w0"), NULL);
   FDTDsimulation->Gamma         = strtod(lookupValue(FDTDsimulation->parameters_key_value_pair, "Gamma"), NULL);
   FDTDsimulation->Lx            = 2 * FDTDsimulation->Nx * FDTDsimulation->Delta;
   FDTDsimulation->Ly            = (FDTDsimulation->Ny-1) * FDTDsimulation->Delta;
   FDTDsimulation->plus_a_index  = FDTDsimulation->Nx + 3*FDTDsimulation->nx/2 + 1;
   FDTDsimulation->minus_a_index = FDTDsimulation->Nx + FDTDsimulation->nx/2 + 1;
   FDTDsimulation->origin_index  = FDTDsimulation->Nx + FDTDsimulation->nx + 1;
   FDTDsimulation->save_chi      = (lookupValue(FDTDsimulation->parameters_key_value_pair, "save_chi") ? \
				   atoi(lookupValue(FDTDsimulation->parameters_key_value_pair, "save_chi")) : 0); //default: off
   FDTDsimulation->save_psi      = (lookupValue(FDTDsimulation->parameters_key_value_pair, "save_psi") ? \
				   atoi(lookupValue(FDTDsimulation->parameters_key_value_pair, "save_psi")) : 0); //default: off
   FDTDsimulation->init_cond     = (lookupValue(FDTDsimulation->parameters_key_value_pair, "init_cond") ? \
	                           atoi(lookupValue(FDTDsimulation->parameters_key_value_pair, "init_cond")) : 0); //default: 0 (unspecified)
   FDTDsimulation->alpha         = (lookupValue(FDTDsimulation->parameters_key_value_pair, "alpha") ? \
	                           atoi(lookupValue(FDTDsimulation->parameters_key_value_pair, "alpha")) : 0);    //default: 0

   //check the validity of parameters
   sanity_check(FDTDsimulation);

   //initialize arrays
   initial_condition(FDTDsimulation);
   boundary_condition(FDTDsimulation);
   initialize_psi(FDTDsimulation);

//   //save memory
//   free_initial_boundary_conditions(FDTDsimulation);

   return FDTDsimulation;
}


void print_initial_condition(grid * simulation)
{
    printf("t=0   ");
    for(int i=0; i<simulation->psit0_size; i++)
        printf("%.2f+%.2fI ", creal(simulation->psit0[i]), cimag(simulation->psit0[i]));
    printf("\n      ");
    for(int i=-simulation->Nx; i<=simulation->Nx; i++)
        printf("x=%.2f ", i*simulation->Delta);
    printf("\n");
}


void print_boundary_condition(grid * simulation)
{
    for(int j=simulation->Ny-1; j>=0; j--)
    {
        printf("          t = %f\n", j*simulation->Delta);
        for(int i=0; i<simulation->psix0_x_size; i++)
        {
            const char * c;
            if(cimag(simulation->psi[j][i])<0)
            {
                c = "";
            }
            else
            {
                c = "+";
            }
            printf("x=%.3f: %.7f%s%.7fI\n", -(simulation->Nx+simulation->nx+1-i)*simulation->Delta, \
                   creal(simulation->psi[j][i]), c, cimag(simulation->psi[j][i]));
        }
        printf("\n");
    }
}


void print_psi(grid * simulation)
{
    for(int j=simulation->Ny-1; j>=0; j--)
    {
        printf("          t = %f\n", j*simulation->Delta);
        for(int i=0; i<simulation->Ntotal; i++)
        {
            const char * c;
            if(cimag(simulation->psi[j][i])<0)
            {
                c = "";
            }
            else
            {
                c = "+";
            }
            //char c[] = (cimag(simulation->psi[j][i])<0?" ":"+");
            printf("x=%.3f: %.7f%s%.7fI\n", -(simulation->Nx+simulation->nx+1-i)*simulation->Delta, \
                   creal(simulation->psi[j][i]), c, cimag(simulation->psi[j][i]));
        }
        printf("\n");
    }
}


//this function stores the computed wavefunction into a file;
//the third argument "part" can be any function converting a 
//complex to a double, e.g., creal, cimag, cabs, etc. 
void save_psi(grid * simulation, const char * filename, double (*part)(complex))
{
    char * str = strdup(filename);
    str = realloc(str, (strlen(filename)+10)*sizeof(char) );

    if(part == &creal)
       strcat(str, ".re.out");
    else if(part == &cimag)
       strcat(str, ".im.out");
    else if(part == &cabs)
       strcat(str, ".abs.out");
    else
    {
       fprintf(stderr, "%s: Warning: default filename is used.\n", __func__);
       strcat(str, ".out");
    }

    FILE * f = fopen(str, "w");

    for(int j=0; j<simulation->Ny; j++)
    {
        for(int i=0; i<simulation->Ntotal; i++)
        {
            fprintf( f, "%.5f ", part(simulation->psi[j][i]) );
        }
        fprintf( f, "\n");
    }

    fclose(f);
    free(str);
}


//this function computes the two-photon wavefunction on the fly and
//then writes to a file, so no extra memory is allocated;
//the third argument "part" can be any function converting a
//complex to a double, e.g., creal, cimag, cabs, etc.
void save_chi(grid * simulation, const char * filename, double (*part)(complex))
{
    char * str = strdup(filename);
    str = realloc(str, (strlen(filename)+15)*sizeof(char) );

    if(part == &creal)
       strcat(str, ".re_chi.out");
    else if(part == &cimag)
       strcat(str, ".im_chi.out");
    else if(part == &cabs)
       strcat(str, ".abs_chi.out");
    else
    {
       fprintf(stderr, "%s: Warning: default filename is used.\n", __func__);
       strcat(str, ".chi.out");
    }

    FILE * f = fopen(str, "w");

    //compute chi(a+Delta, a+Delta+tau, t) with tau=i*Delta and t=j*Delta:
    //to make all terms in chi well-defined requires 0 <= i <= Nx-nx/2.
    //Similarly, j must >= simulation->minus_a_index in order to let signal from the 1st qubit reach the boundary;
    //put it differently, one cannot take data before the first light cone intersects with the boundary x=Nx*Delta.
    complex chi = 0;
    for(int j=(simulation->Nx+simulation->nx/2+1); j<=simulation->Ny; j++)
    {
        for(int i=0; i<=simulation->Nx-simulation->nx/2; i++)
        {
	    chi = cexp(I * simulation->k * (simulation->nx+2+i-2*j) * simulation->Delta) \
		  -sqrt(simulation->Gamma)/2.0 * \
		  (  simulation->psi[j-(simulation->nx+i+1)][simulation->minus_a_index-i] \
		   - simulation->psi[j-(i+1)][simulation->plus_a_index-i] \
		   + simulation->psi[j-(simulation->nx+1)][simulation->minus_a_index+i] \
		   - simulation->psi[j-1][simulation->plus_a_index+i] \
		  );
            fprintf( f, "%.4f ", part(chi) );
        }
        fprintf( f, "\n");
    }

    fclose(f);
    free(str);
}


void print_grid(grid * simulation)
{
    printf("nx = %d\n", simulation->nx); 
    printf("Nx = %d\n", simulation->Nx); 
    printf("Ntotal = %d\n", simulation->Ntotal); 
    printf("Ny = %d\n", simulation->Ny); 
    printf("Delta = %.3f\n", simulation->Delta); 
    printf("k = %.3f\n", simulation->k); 
    printf("w0 = %.3f\n", simulation->w0); 
    printf("Gamma = %.3f\n", simulation->Gamma); 
    printf("Lx = %.3f\n", simulation->Lx); 
    printf("Ly = %.3f\n", simulation->Ly);
 
    for(int j=simulation->Ny-1; j>=0; j--)
    {
        for(int i=0; i<simulation->Ntotal; i++)
        {
            printf("%.2f+%.2fI ", creal(simulation->psi[j][i]), cimag(simulation->psi[j][i]));
        }
        printf("\n");
    }
}
