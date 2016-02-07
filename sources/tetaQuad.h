#ifdef _WIN32
    #define _USE_MATH_DEFINES
#endif

#include <stdio.h>
#include <math.h>

const size_t MAX_DIVISIONS = 21;
const size_t MAX_ITERATIONS = 21;
const double PRECISION_DELTA = 0.00000001;
const double DELTA = 16.0;

double potential(double r)
{
    return 1.0/r;
}

double function(double r, double b, double E)
{  
    return 1.0 / (pow(r, 2) * sqrt(1 - (pow(b, 2) / pow(r, 2)) - (potential(r) / E)));
}

double finite_integral(double up, double to, double b, double E)
{
    double cur_step,
           step,
           prev_sum,
           partial_sum,
           f_x;
    size_t i;

    prev_sum = 0.0;

    for(i = 0; i != MAX_DIVISIONS; ++i)
    {
        step = (to-up)/pow(2, i);
        partial_sum = 0.0;

        for(cur_step = up + step; cur_step <= to; cur_step += step) {
            f_x = step * function(cur_step - (step/2.0), b, E);
            
            // when f_x is nan
            // if f_x is nan, f_x != f_x will be true
            if(f_x == f_x) {  
                partial_sum = partial_sum + f_x;
            }
        }

        if(prev_sum != 0.0 && fabs(prev_sum - partial_sum) < PRECISION_DELTA) break;
        prev_sum = partial_sum;
    }
    
    return partial_sum;
}

double integral_to_infinite(double a, double b, double E)
{
    double to,
           prev_res,
           partial_res;
    size_t i;

    prev_res = 0.0;
    
    for(i = 1; i != pow(2, MAX_ITERATIONS); ++i)
    {
        to = a + DELTA;
        partial_res = finite_integral(a, to, b, E);

        if(prev_res != 0.0 && partial_res != 0.0 && partial_res < PRECISION_DELTA)
        {
            prev_res += partial_res;
            break;
        }

        // fprintf(stdout, "i:%zu\ta:%f\tto:%f\tparial:%f\n", i, a, to, partial_res);

        prev_res += partial_res;
        a = to;
    }

    return (2.0 * b) * prev_res;
}

double to_degrees(double radians) {
    return 180.0 - radians * (180.0 / M_PI);
}