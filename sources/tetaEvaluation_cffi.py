from cffi import FFI
from matplotlib import pyplot as plt
from matplotlib import patches as mpatches
from ctypes import c_double
from math import asin, sqrt, degrees


def analytic_evaluation(var_b, var_e):
    """Analytic function of theta integral."""
    return 2.0 * asin(1 / (sqrt(1 + 4 * float(var_b) ** 2 * float(var_e) ** 2)))


def range_f(start, end, step):
    """Create a list of double values."""
    end = c_double(end)
    ffi = FFI()
    start = c_double(start)
    while start.value <= end.value:
        yield ffi.cast("double", start.value)
        start.value = start.value + c_double(step).value


def main():
    """Program main."""
    ffi = FFI()
    ffi.cdef("""
double integral_to_infinite(double a, double b, double E);
double to_degrees(double radians);

""")
    # C = ffi.dlopen(None)
    lib = ffi.verify("""
#include "tetaQuad.h"
""", include_dirs=["."], extra_compile_args=["-O3"])

    var_e = ffi.cast("double", 0.1)
    var_a = ffi.cast("double", 0.0)

    points_x = []
    points_y = []
    points_y_a = []

    print("Calculus may take some time...")

    for var_b in range_f(0.0, 100, 0.5):
        points_x.append(var_b)
        rad = lib.integral_to_infinite(var_a, var_b, var_e)
        theta = lib.to_degrees(abs(rad))
        analytic = degrees(analytic_evaluation(var_b, var_e))

        print("b = {}\ttheta = {}\tanalytic = {}".format(
            float(var_b), theta, analytic))

        points_y.append(theta)
        points_y_a.append(analytic)

    plt.ylabel('teta')
    plt.xlabel('b')
    # plt.plot(points_x, points_y, 'r--')
    plt.plot(points_x, points_y, 'ro')
    plt.plot(points_x, points_y_a, 'b--')
    # plt.plot(points_x, points_y_a, 'bo')
    red_line = mpatches.Patch(color='red', label='theta value')
    blue_line = mpatches.Patch(color='blue', label='analytic value')
    plt.legend(handles=[red_line, blue_line])
    plt.grid(True)
    plt.show()


if __name__ == '__main__':
    main()
