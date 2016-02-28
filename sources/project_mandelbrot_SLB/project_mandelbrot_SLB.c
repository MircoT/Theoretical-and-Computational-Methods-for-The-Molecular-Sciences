#include <stdlib.h>  // required by malloc
#include <string.h>
#include <stdio.h>
#include <stddef.h>  // required by offsetof
#include <mpi.h>

#define PRINT_MATRIX 0
#define LOG 0

typedef unsigned char BYTE;
typedef unsigned short DATA_TYPE;

typedef struct mandelbrot_params_s 
{
    unsigned int start_x;
    unsigned int start_y;
    unsigned int size_x;
    unsigned int size_y;
} mandelbrot_params;

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
    void printMatrix(DATA_TYPE *matrix, unsigned int width, unsigned int height)
    {   
        unsigned int y, x;
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

    int num_groups_x = -1, 
        num_groups_y = -1;
    
    double start = 0.0, 
           end = 0.0;

    /*----- Default values -----*/
    unsigned int width = 1920,
                 height = 1080,
                 max_iterations = 10000;
    
    /*----- Start MPI environment -----*/
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /**
     * Arguments:
     *
     * - argv[1] -> NxM (grid)(required)
     * - argv[2] -> NxM (screen resolution)(optional, has default value)
     * - argv[3] -> N (number of iterations)(optional, has default value)
     * 
     */

    /*----- START Args parsing -----*/
    if (argc == 1)
    {
        fprintf(stdout, ">> An input grid is required to run this program, for example 2x3, 3x2, 2x8, 4x4, 8x2\n");
        MPI_Abort(MPI_COMM_WORLD, 3);
    }

    /** Grid **/
    ok = sscanf( argv[1], "%dx%d", &num_groups_x, &num_groups_y);

    if (ok != 2)
    {
        fprintf(stdout, ">> Something went wrong during grid parsing...\n");
        MPI_Abort(MPI_COMM_WORLD, 5);
    }

    if (argc >= 3)
    {   
        /** Screen resolution **/
        ok = sscanf( argv[2], "%dx%d", &width, &height);
        if (ok != 2)
        {
            fprintf(stdout, ">> Something went wrong during image resolution parsing...\n");
            MPI_Abort(MPI_COMM_WORLD, 6);
        }
    }

    if (argc >= 4)
    {   
        /** Number of iterations **/
        ok = sscanf( argv[3], "%d", &max_iterations);
        if (ok != 1)
        {
            fprintf(stdout, ">> Something went wrong during max iterations parsing...\n");
            MPI_Abort(MPI_COMM_WORLD, 7);
        }
    }

    if (num_groups_x * num_groups_y > size)
    {
        fprintf(stdout, ">> You need %d processes in a grid %s and you have %d processes...\n", num_groups_x * num_groups_y, argv[1], size);
        MPI_Abort(MPI_COMM_WORLD, 8);
    }
    /*----- END Args parsing -----*/

    /*----- Message MODEL -----*/
    const int nitems = 4;
    int blocklengths[4] = {1, 1, 1, 1};
    MPI_Datatype types[4] = {MPI_UNSIGNED, MPI_UNSIGNED, MPI_UNSIGNED, MPI_UNSIGNED};
    MPI_Datatype mpi_mandelbrot_params;
    MPI_Aint offsets[4];

    offsets[0] = offsetof(mandelbrot_params, start_x);
    offsets[1] = offsetof(mandelbrot_params, start_y);
    offsets[2] = offsetof(mandelbrot_params, size_x);
    offsets[3] = offsetof(mandelbrot_params, size_y);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_mandelbrot_params);
    MPI_Type_commit(&mpi_mandelbrot_params);
    /*----- END Message MODEL -----*/

    /*----- MPI TYPE -----*/
    MPI_Datatype current_mpi_type = MPI_BYTE;

    switch(sizeof(DATA_TYPE)) {
        case 4 :
            current_mpi_type = MPI_UNSIGNED;
        case 2 :
            current_mpi_type = MPI_UNSIGNED_SHORT;
        default :
            current_mpi_type = MPI_BYTE;
    }
    /*----- END MPI TYPE -----*/

    #if LOG
        fprintf(stdout, ">> Process rank(%d) online - tot processes (%d)\n", rank, size);
    #endif

    if (rank == 0)
    {       
        fprintf(stdout, ">>> Starting SLB Algorithm...\n");
        fprintf(stdout, ">>> num groups: %dx%d\n", num_groups_x, num_groups_y);
        fprintf(stdout, ">>> image size: %dx%d\n", width, height);
        fprintf(stdout, ">>> max iterations: %d\n", max_iterations);
        
        const short num_elm_x = width / num_groups_x;
        const short num_elm_y = height / num_groups_y;

        
        mandelbrot_params *params_container = NULL;
        params_container = (mandelbrot_params*) malloc(sizeof(mandelbrot_params) * (num_groups_x * num_groups_y));

        #if LOG
            fprintf(stdout, ">>> elms per groups: %dx%d\n", num_elm_x, num_elm_y);
        #endif

        start = MPI_Wtime();

        unsigned int y = 0,
                     x = 0;

        /*----- Send jobs -----*/
        for (y = 0; y != num_groups_y; ++y)
        {
            for (x = 0; x != num_groups_x; ++x)
            {
                if (x != 0 || y != 0)
                {
                    unsigned int start_x = x * num_elm_x;
                    unsigned int start_y = y * num_elm_y;
                    unsigned int size_x = 0;
                    unsigned int size_y = 0;

                    if (x == (num_groups_x - 1))
                        size_x = num_elm_x + (width % num_elm_x);
                    else
                        size_x = num_elm_x;

                    if (y == (num_groups_y - 1))
                        size_y = num_elm_y + (height % num_elm_y);
                    else
                        size_y = num_elm_y;

                    unsigned int process_num = x + y*num_groups_x;

                    #if LOG
                        fprintf(stdout, ">>> s_x: %d\ts_y: %d\tsize_x: %d\tsize_y: %d\tdest_proc: %d\n",
                            start_x, start_y, size_x, size_y, process_num);
                    #endif

                    params_container[process_num].start_x = start_x;
                    params_container[process_num].start_y = start_y;
                    params_container[process_num].size_x = size_x;
                    params_container[process_num].size_y = size_y;
                    
                    MPI_Send(&params_container[process_num], 1, mpi_mandelbrot_params, process_num, 0, MPI_COMM_WORLD);
                }
                
            }
        }

        /*----- Do MASTER job -----*/
        params_container[0].start_x = 0;
        params_container[0].start_y = 0;
        params_container[0].size_x = num_elm_x;
        params_container[0].size_y = num_elm_y;
        
        DATA_TYPE *master_buffer = NULL;
        master_buffer = gen_mandelbrot_set(0, 0, max_iterations, num_elm_x, num_elm_y, width, height); 
        
        /*----- Receive results -----*/
        int process_num = 0;

        DATA_TYPE *final_matrix = NULL;
        DATA_TYPE *buffer = NULL;
        final_matrix = (DATA_TYPE*) malloc(sizeof(DATA_TYPE) * (width * height));

        for(process_num = 0; process_num < (num_groups_x * num_groups_y); ++process_num)
        {   
            mandelbrot_params *cur_params = &params_container[process_num];

            if(process_num != 0)
            {   
                int num_elms = cur_params->size_x * cur_params->size_y;

                if(buffer != NULL) buffer = (DATA_TYPE*) realloc(buffer, sizeof(DATA_TYPE) * num_elms);
                else buffer = (DATA_TYPE*) malloc(sizeof(DATA_TYPE) * num_elms);
                
                MPI_Recv(&buffer[0], num_elms, current_mpi_type, process_num, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }

            unsigned int row = 0;

            for (row = 0; row != cur_params->size_y; ++row)
            {
                unsigned int offset = row * cur_params->size_x;
                unsigned int final_index = cur_params->start_x + (cur_params->start_y + row) * width;
                
                if(process_num != 0)
                {
                    memcpy(
                        final_matrix + final_index,
                        buffer + offset,
                        sizeof(DATA_TYPE) * cur_params->size_x
                    );
                }
                else
                {
                    memcpy(
                        final_matrix + final_index,
                        master_buffer + offset,
                        sizeof(DATA_TYPE) * cur_params->size_x
                    );
                }
            }   
        }

        end = MPI_Wtime();

        #if PRINT_MATRIX
            printMatrix(final_matrix, width, height);
        #endif  

        fprintf(stdout, ">>> Done!\n");
        fprintf(stdout, ">>> Elapsed time is %f\n", end - start );

        /*----- CLEAN -----*/
        free(master_buffer);
        free(buffer);
        free(params_container);
        free(final_matrix);
    }
    else if(rank < num_groups_x * num_groups_y)
    {   
        // int run = 1;
        mandelbrot_params recv_params;
        
        MPI_Recv(&recv_params, 1, mpi_mandelbrot_params, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        #if LOG
            fprintf(stdout, ">>>> Process rank(%d) received the job - tot process: %d\n", rank, size);
            fprintf(stdout, ">>>> Process rank(%d) will calculate image\n>>>>\tfrom (%d,%d)\n>>>>\twith size (%d,%d)\n>>>>\tof the image (%d,%d)\n",
                rank, recv_params.start_x, recv_params.start_y, recv_params.size_x, recv_params.size_y, width, height);
        #endif

        int num_elms = recv_params.size_x * recv_params.size_y;
        DATA_TYPE *result = gen_mandelbrot_set(recv_params.start_x, recv_params.start_y, max_iterations, recv_params.size_x, recv_params.size_y, width, height);

        #if PRINT_MATRIX
            printMatrix(result, recv_params.size_x, recv_params.size_y);   
        #endif  
        
        #if LOG
            fprintf(stdout, ">>>> Process rank(%d) send %d elms\n", rank, num_elms);
        #endif

        MPI_Send(&result[0], num_elms, current_mpi_type, 0, 0, MPI_COMM_WORLD);

        /*----- CLEAN -----*/
        free(result);
    }

    #if LOG
        fprintf(stdout, ">> Process rank(%d) exiting...\n", rank);
    #endif
    
    MPI_Finalize();
    return 0;
}
