# FDTD: solving 1+1D delay PDE
This code is intended to solve 1+1D delay PDE which emerges in waveguide QED problems.

## Requirements
None. This code is written in pure C and conforming the gnu99 standard, so it can be compiled on any modern OS with standard C libraries.

## Features
* Fast and efficient
* Valgrind-clean (no memory leak)
* One of the first few attempts to numerically solve PDE delayed in both dimensions

## Installation
A makefile is provided. After cloning the git repo or downloading the source code, simply type `make` in the same folder to compile, and an executable named `FDTD` will be generated.

## Usage
`./FDTD input_filename`

## Mandatory input parameters
Only 7 are required: `nx`, `Nx`, `Ny`, `Delta`, `k`, `w0`, and `Gamma`. The first 4 are simulation-related and the rest are physics-related. See the comments in `grid.h` for the layout of the grid and details of various parameters. A Python script is provided in the `utilities` folder for users' convenience.

Currently only one kind of initial condition (two-photon plane wave) is built in. Other forms are planned to be included in the future.

## Output
Currently two files will be generated: `input_filename.re.out` and `input_filename.im.out` (real and imaginary parts of the wavefunction, respectively). A Mathematica notebook is provided in the `utilities` folder for simple plotting purposes.

## License
This code will be released under [the WTFPL license]. Copyright (c) 2016 by Leo Fang <leofang@phy.duke.edu>



[the WTFPL license](http://www.wtfpl.net)
