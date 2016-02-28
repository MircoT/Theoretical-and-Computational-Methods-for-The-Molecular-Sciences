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
    unsigned int _exit;
} mandelbrot_params;

void gen_mandelbrot_set(
						DATA_TYPE *point_list,
						const unsigned int start_x,
						const unsigned int start_y,
						const unsigned int max_iterations,
						unsigned int size_x,
						unsigned int size_y,
						unsigned int img_size_x,
						unsigned int img_size_y
                        )
{
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

/**
 * Return the index of the available worker
 * @param  workers constant list of the worker params
 * @param  worker_num         size of the list
 * @return                    index of the available space (NULL index in the list)
 */
int worker_available(short* const workers, const unsigned int worker_num)
{   
    int i = 0;
    for (i = 0; i != worker_num; ++i)
    {
        if(workers[i] == -1) return i;
    }
    return -1;
}

/**
 * return 1 if there aren't workers with a job
 * @param  workers constant list of the worker params
 * @param  worker_num         size of the list
 * @return                    0 (False) if all workers are free, 1 otherwise
 */
int someone_busy(short* const workers, const unsigned int worker_num)
{   
    int i = 0;
    for (i = 0; i != worker_num; ++i)
    {
        if(workers[i] != -1) return 1;
    }
    return 0;
}

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
    double k = 1.0;
    
    /*----- Start MPI environment -----*/
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /**
     * Arguments:
     *
     * - argv[1] -> NxM (grid)(required)
     * - argv[2] -> N (K)(required)
     * - argv[3] -> NxM (screen resolution)(optional, has default value)
     * - argv[4] -> N (number of iterations)(optional, has default value)
     * 
     */

    /*----- START Args parsing -----*/
    if (argc < 3)
    {
        fprintf(stdout, ">> An input grid and a value K are required to run this program, grid can be for example 2x3, 3x2, 2x8, 4x4, 8x2 and K = [0.25, 0.5, 0.75, 1]\n");
        MPI_Abort(MPI_COMM_WORLD, 3);
    }

    /** Grid **/
    ok = sscanf( argv[1], "%dx%d", &num_groups_x, &num_groups_y);

    if (ok != 2)
    {
        fprintf(stdout, ">> Something went wrong during grid parsing...\n");
        MPI_Abort(MPI_COMM_WORLD, 5);
    }

    /** K **/
    ok = sscanf( argv[2], "%lf", &k);

    if (ok != 1)
    {
        fprintf(stdout, ">> Something went wrong during K parsing...\n");
        MPI_Abort(MPI_COMM_WORLD, 6);
    }

    if (argc >= 4)
    {   
        /** Screen resolution **/
        ok = sscanf( argv[3], "%dx%d", &width, &height);
        if (ok != 2)
        {
            fprintf(stdout, ">> Something went wrong during image resolution parsing...\n");
            MPI_Abort(MPI_COMM_WORLD, 7);
        }
    }

    if (argc >= 5)
    {   
        /** Number of iterations **/
        ok = sscanf( argv[4], "%d", &max_iterations);
        if (ok != 1)
        {
            fprintf(stdout, ">> Something went wrong during max iterations parsing...\n");
            MPI_Abort(MPI_COMM_WORLD, 8);
        }
    }

    if (num_groups_x * num_groups_y > size)
    {
        fprintf(stdout, ">> You need %d processes in a grid %s and you have %d processes...\n", num_groups_x * num_groups_y, argv[1], size);
        MPI_Abort(MPI_COMM_WORLD, 9);
    }

    if (k != 0.25 && k != 0.5 && k != 0.75 && k != 1.0)
    {
        fprintf(stdout, ">> K must be one of these values: [0.25, 0.5, 0.75, 1]\n");
        MPI_Abort(MPI_COMM_WORLD, 10);
    }
    /*----- END Args parsing -----*/

    /*----- Message MODEL -----*/
    const int nitems = 5;
    int blocklengths[5] = {1, 1, 1, 1, 1};
    MPI_Datatype types[5] = {MPI_UNSIGNED, MPI_UNSIGNED, MPI_UNSIGNED, MPI_UNSIGNED, MPI_UNSIGNED};
    MPI_Datatype mpi_mandelbrot_params;
    MPI_Aint offsets[5];

    offsets[0] = offsetof(mandelbrot_params, start_x);
    offsets[1] = offsetof(mandelbrot_params, start_y);
    offsets[2] = offsetof(mandelbrot_params, size_x);
    offsets[3] = offsetof(mandelbrot_params, size_y);
    offsets[4] = offsetof(mandelbrot_params, _exit);

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
        fprintf(stdout, ">>> Starting DLB Algorithm...\n");
        fprintf(stdout, ">>> num groups: %dx%d\n", num_groups_x, num_groups_y);
        fprintf(stdout, ">>> image size: %dx%d\n", width, height);
        fprintf(stdout, ">>> max iterations: %d\n", max_iterations);
        fprintf(stdout, ">>> K: %f\n", k);
        
        const short num_elm_x = k * width / num_groups_x;
        const short num_elm_y = k * height / num_groups_y;

        mandelbrot_params *params_container = NULL;
        unsigned int params_container_size = num_groups_x * num_groups_y - 1;
        params_container = (mandelbrot_params*) malloc(sizeof(mandelbrot_params) * params_container_size);
        short params_container_flags[params_container_size];

        int cont_i = 0;
        for (cont_i = 0; cont_i != params_container_size; ++cont_i)
        {
            params_container_flags[cont_i] = -1;
        }

        int process_num = 0;
        for(process_num = 1; process_num < (num_groups_x * num_groups_y); ++process_num)
        {   
            params_container[process_num - 1].size_x = 0;  
            params_container[process_num - 1].size_y = 0;
            params_container[process_num - 1].start_x = 0;
            params_container[process_num - 1].start_y = 0;
            params_container[process_num - 1]._exit = 0;
        }

        #if LOG
            fprintf(stdout, ">>> elms per groups: %dx%d\n", num_elm_x, num_elm_y);
        #endif

        DATA_TYPE *final_matrix = (DATA_TYPE*) malloc(sizeof(DATA_TYPE) * (width * height));
        MPI_Status status;

        start = MPI_Wtime();

        unsigned int y = 0,
                     x = 0;

        for (y = 0; y < height; y += num_elm_y)
        {
            for (x = 0; x < width; x += num_elm_x)
            {
                int start_x = x;
                int start_y = y;
                int size_x = num_elm_x;
                int size_y = num_elm_y;

                if (start_x + size_x > width)
                    size_x = width % num_elm_x;

                if (start_y + size_y > height)
                    size_y = height % num_elm_y;

                #if LOG
                    fprintf(stdout, ">>> s_x: %d\ts_y: %d\tsize_x: %d\tsize_y: %d\tworker_available: %d\n",
                        start_x, start_y, size_x, size_y, worker_available(params_container_flags, params_container_size));
                #endif

                if(worker_available(params_container_flags, params_container_size) != -1)
                {   
                    int free_worker = worker_available(params_container_flags, params_container_size);
                    
                    params_container[free_worker].start_x = start_x;
                    params_container[free_worker].start_y = start_y;
                    params_container[free_worker].size_x = size_x;
                    params_container[free_worker].size_y = size_y;
                    params_container[free_worker]._exit = 0;

                    params_container_flags[free_worker] = 0;

                    MPI_Send(&params_container[free_worker], 1, mpi_mandelbrot_params, (free_worker+1), 0, MPI_COMM_WORLD);               
                }
                else
                {   
                    DATA_TYPE *buffer = NULL;
                    while(worker_available(params_container_flags, params_container_size) == -1)
                    {
                        int num_elems = -1;

                        MPI_Probe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
                        MPI_Get_count(&status, current_mpi_type, &num_elems);

                        buffer = (DATA_TYPE*) realloc(buffer, sizeof(DATA_TYPE) * num_elems);
                        #if LOG
                            fprintf(stdout, "HERE %d\n", num_elems);
                        #endif
                        MPI_Recv(&buffer[0], num_elems, current_mpi_type, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                        #if LOG
                            fprintf(stdout, "HERE2\n");
                        #endif

                        unsigned int row = 0;
                        mandelbrot_params cur_params = params_container[(status.MPI_SOURCE - 1)];

                        for (row = 0; row != cur_params.size_y; ++row) {
                            unsigned int offset = row * cur_params.size_x;
                            unsigned int final_index = cur_params.start_x + (cur_params.start_y + row) * width;
                            memcpy(
                                final_matrix + final_index,
                                buffer + offset,
                                sizeof(DATA_TYPE) * cur_params.size_x
                            );
                        }

                        #if LOG
                            fprintf(stdout, "Received result from process %d, now it will have a new job...\n", status.MPI_SOURCE);
                        #endif

                        params_container_flags[(status.MPI_SOURCE - 1)] = -1;
                    }

                    /*----- CLEAN -----*/
                    free(buffer);

                    #if LOG
                        fprintf(stdout, ">>>> New job for process (%d) -> s_x: %d\ts_y: %d\tsize_x: %d\tsize_y: %d\tworker_available: %d\n",
                            status.MPI_SOURCE, start_x, start_y, size_x, size_y, worker_available(params_container_flags, params_container_size));
                    #endif

                    int free_worker = worker_available(params_container_flags, params_container_size);
                    
                    params_container[free_worker].start_x = start_x;
                    params_container[free_worker].start_y = start_y;
                    params_container[free_worker].size_x = size_x;
                    params_container[free_worker].size_y = size_y;
                    params_container[free_worker]._exit = 0;

                    params_container_flags[free_worker] = 0;

                    MPI_Send(&params_container[free_worker], 1, mpi_mandelbrot_params, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
                }
            }
        }

        DATA_TYPE *buffer = NULL;
        while(someone_busy(params_container_flags, params_container_size))
        {
            int num_elems = -1;

            #if LOG
                fprintf(stdout, ">>> Wait for results... someone is busy...\n");
            #endif
            MPI_Probe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
            MPI_Get_count(&status, current_mpi_type, &num_elems);

            buffer = (DATA_TYPE*) realloc(buffer, sizeof(DATA_TYPE) * num_elems);
            MPI_Recv(&buffer[0], num_elems, current_mpi_type, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            unsigned int row = 0;
            mandelbrot_params cur_params = params_container[(status.MPI_SOURCE - 1)];

            for (row = 0; row != cur_params.size_y; ++row) {
                unsigned int offset = row * cur_params.size_x;
                unsigned int final_index = cur_params.start_x + (cur_params.start_y + row) * width;
                memcpy(
                    final_matrix + final_index,
                    buffer + offset,
                    sizeof(DATA_TYPE) * cur_params.size_x
                );
            }

            #if LOG
                fprintf(stdout, "Received result from process %d, now it will have a new job...\n", status.MPI_SOURCE);
            #endif

            params_container_flags[(status.MPI_SOURCE - 1)] = -1;

            
        }
        /*----- CLEAN -----*/
        free(buffer);

        #if PRINT_MATRIX
            printMatrix(final_matrix, width, height);
        #endif

        end = MPI_Wtime();

        fprintf(stdout, ">>> Done!\n");
        fprintf(stdout, ">>> Elapsed time is %f\n", end - start );

        /*----- CLEAN -----*/
        for(process_num = 1; process_num <(num_groups_x * num_groups_y); ++process_num)
        {    
            mandelbrot_params exit_params;
            exit_params.start_x = 0;
            exit_params.start_y = 0;
            exit_params.size_x = 0;
            exit_params.size_y = 0;
            exit_params._exit = 1;
            MPI_Send(&exit_params, 1, mpi_mandelbrot_params, process_num, 0, MPI_COMM_WORLD);
        }
        free(params_container);
        free(final_matrix);

    }
    else if(rank < num_groups_x * num_groups_y)
    {   
        int run = 1;

        while(run)
        {
            mandelbrot_params recv_params;
            MPI_Recv(&recv_params, 1, mpi_mandelbrot_params, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if(recv_params._exit == 0)
            {
                #if LOG
                    fprintf(stdout, ">>>> Process rank(%d) received the job - tot process: %d\n", rank, size);
                    fprintf(stdout, ">>>> Process rank(%d) will calculate image\n>>>>\tfrom (%d,%d)\n>>>>\twith size (%d,%d)\n>>>>\tof the image (%d,%d)\n",
                        rank, recv_params.start_x, recv_params.start_y, recv_params.size_x, recv_params.size_y, width, height);
                #endif

                int num_elms = recv_params.size_x * recv_params.size_y;

                DATA_TYPE *result_buf = (DATA_TYPE*) malloc(sizeof(DATA_TYPE) * num_elms);
                
                gen_mandelbrot_set(result_buf, recv_params.start_x, recv_params.start_y, max_iterations, recv_params.size_x, recv_params.size_y, width, height);

                #if PRINT_MATRIX
                    printMatrix(result_buf, recv_params.size_x, recv_params.size_y);   
                #endif  
                
                #if LOG
                    fprintf(stdout, ">>>> Process rank(%d) send %d elements\n", rank, num_elms);
                #endif

                MPI_Send(&result_buf[0], num_elms, current_mpi_type, 0, 0, MPI_COMM_WORLD); 

                /*----- CLEAN -----*/
                free(result_buf);
            }
            else
            {   
                #if LOG
                    fprintf(stdout, ">> Process rank(%d) received order to exit!\n", rank);
                #endif
                run = 0;
            }     
        }
    }

    #if LOG
        fprintf(stdout, ">> Process rank(%d) exiting...\n", rank);
    #endif
    
    MPI_Finalize();
    return 0;
}
