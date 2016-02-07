#include <stdlib.h>  // required by malloc
#include <stdio.h>
#include <mpi.h>

#define PRINT_MATRIX 0

typedef unsigned char BYTE;
typedef unsigned short DATA_TYPE;

DATA_TYPE* gen_mandelbrot_set(
                         const unsigned int start_x,
                         const unsigned int start_y,
                         const unsigned int max_iterations,
                         unsigned int size_x,
                         unsigned int size_y,
                         unsigned int img_size_x,
                         unsigned int img_size_y
                         )
{
    DATA_TYPE *point_list;
    point_list = (DATA_TYPE*) malloc(sizeof(DATA_TYPE) * size_x * size_y);
    
    unsigned int Px = 0;
    unsigned int Py = 0;

    for(Py = start_y; Py != start_y + size_y; ++Py)
    {
        for(Px = start_x; Px != start_x + size_x; ++Px)
        {
            double x0 = ((double) Px * 3.5 / (double) img_size_x) - 2.5;
            double y0 = ((double) Py * 2.0 / (double) img_size_y) - 1.0;
            double x = 0.0;
            double y = 0.0;
            double xTemp = 0.0;
            DATA_TYPE iteration = 0;
            
            while( (x*x + y*y) < 2*2 && iteration < max_iterations)
            {
                xTemp = x*x - y*y + x0;
                y = 2*x*y + y0;
                x = xTemp;
                
                iteration++;
            }
            
            point_list[(Px - start_x) + (Py - start_y) * size_x] = iteration;
        }
    }
    
    return point_list;
}

#if PRINT_MATRIX
    void printMatrix(DATA_TYPE *matrix, int width, int height)
    {   
        int y, x;
        fprintf(stdout, "\n");
        for (y = 0; y != height; ++y) {
            for (x = 0; x != width; ++x) {
                fprintf(stdout, "%i\t", matrix[x + y*width]);
            }
            fprintf(stdout, "\n");
        }
        fprintf(stdout, "\n");
    }
#endif

int main (int argc, char** argv)
{
    int rank = -1, 
        size = -1, 
        ok = 0;

    DATA_TYPE *mandelbrot_matrix;
    
    unsigned int width = 1920,
                 height = 1080,
                 max_iteration = 10000;

    double start, end; 

    if (argc >= 2)
    {
        ok = sscanf( argv[1], "%dx%d", &width, &height);
        if (ok != 2)
        {
            fprintf(stdout, ">>> Something went wrong during image resolution parsing...\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    if (argc >= 3)
    {
        ok = sscanf( argv[2], "%d", &max_iteration);
        if (ok != 1)
        {
            fprintf(stdout, ">>> Something went wrong during max iterations parsing...\n");
            MPI_Abort(MPI_COMM_WORLD, 2);
        }
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    fprintf(stdout, ">>> I'm process rank(%d) - tot process: %d\n", rank, size);
    fprintf(stdout, ">>> Starting serial algorithm...\n");
    
    start = MPI_Wtime(); 
    mandelbrot_matrix = gen_mandelbrot_set(0, 0, max_iteration, width, height, width, height);
    end = MPI_Wtime();

    #if PRINT_MATRIX
        if(rank == 0)
            printMatrix(mandelbrot_matrix, width, height);
    #endif

    fprintf(stdout, ">>> Done!\n");
    fprintf(stdout, ">>> Elapsed time is %f\n", end - start);
    
    free(mandelbrot_matrix);

    MPI_Finalize();
    return 0;
}
