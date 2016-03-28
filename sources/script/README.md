## Installation

Put all files in your home directory inside the bin folder:

```bash
find . -type f -not -name "*.md" | xargs -I % cp % $HOME/bin
```

**NOTE**: your `$HOME/bin` have to be one of the folder in `PATH` otherwise you have to add it in your bash scrip or temporarily with `export PATH=$PATH:$HOME/bin`.

## How to

First you have to set up the source folder, for example if you are in the main directory of this repository:

```bash
git sub setSource sources/
```

Then you can have the list of the projects with `git sub getList`:

```bash
$ git sub getList
Your projects are:
  0) project_mandelbrot_DLB
  1) project_mandelbrot_SLB
  2) project_mandelbrot_serial
  3) script
  4) tetaEvaluation.py
  5) tetaEvaluation_cffi.py
  6) tetaQuad.h
```

Only an MPI project can be launched and so only the first 3 projects are good because they represent a folder with a single *C file* that is the *MPI source* code. You can use the number of the project or the complete name and you will see the results on screen. The log of the sub process will be in the same directory of the repository, in a folder named *log*, if you launch the sub from there and also the script used for the submission (named *cur_sub.sh*).

Here some example of submission commands:

```bash
git sub -n 1 -p 1 project_mandelbrot_serial 128x128
# The previous command is equal to:
git sub -n 1 -p 1 2 128x128

# project_mandelbrot_SLB example
git sub -n 2 -p 1 1 2x1 128x128

# project_mandelbrot_DLB example
git sub -n 2 -p 1 0 2x1 0.25 128x128

```