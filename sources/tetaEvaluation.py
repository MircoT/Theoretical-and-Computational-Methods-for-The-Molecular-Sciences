from matplotlib import pyplot as plt
from matplotlib import patches as mpatches
from math import sqrt, degrees, asin

MAX_DIVISIONS = 21
MAX_ITERATIONS = 21
PRECISION_DELTA = 0.00000002
DELTA = 16.0


def analytic_evaluation(var_b, var_e):
    """Analytic function of theta integral."""
    return 2.0 * asin(1 / (sqrt(1 + 4 * var_b ** 2 * var_e ** 2)))


def integral_func(var_r, var_b, var_e):
    """Integral to calculate."""
    potential = lambda var_r: 1.0 / var_r
    try:
        return 1.0 / (
            var_r**2 * sqrt(1 - (var_b**2 / var_r**2) - (
                potential(var_r) / var_e)))
    except ValueError:
        return 0.0
    except ZeroDivisionError:
        return 0.0


def finite_integral(var_up, var_to, var_b, var_e):
    """Finite Riemann integral."""
    prev_sum = 0.0

    for i in range(MAX_DIVISIONS):

        step = (var_to-var_up) / 2**i
        partial_sum = 0.0
        cur_step = var_up + step

        while cur_step <= var_to:

            f_x = step * integral_func(cur_step - (step/2.0), var_b, var_e)
            partial_sum += f_x
            cur_step += step

        if prev_sum != 0.0 and abs(prev_sum - partial_sum) < PRECISION_DELTA:
            break

        prev_sum = partial_sum

    return partial_sum


def integral_to_infinite(var_up, var_b, var_e):
    """Riemann integral open at right plus final multiplication."""
    prev_res = 0.0

    for i in range(MAX_ITERATIONS):

        var_to = var_up + DELTA
        partial_res = finite_integral(var_up, var_to, var_b, var_e)

        if prev_res != 0.0 and partial_res != 0.0\
                and partial_res < PRECISION_DELTA:
            break

        prev_res += partial_res
        var_up = var_to

    return (2.0 * var_b) * prev_res


var_a = 0.0
var_e = 0.1

points_x = []
points_y = []
points_y_a = []

print("Calculus may take some time...")

for var_b in range(0, 16):
    points_x.append(var_b)

    final_theta = 180.0 - degrees(abs(integral_to_infinite(var_a, var_b, var_e)))
    analytic = degrees(analytic_evaluation(var_b, var_e))

    print("b = {}\ttheta = {}\tanalytic = {}".format(var_b, final_theta, analytic))

    points_y.append(final_theta)
    points_y_a.append(analytic)

plt.ylabel('teta')
plt.xlabel('b')
plt.plot(points_x, points_y, 'r--')
plt.plot(points_x, points_y, 'ro')
plt.plot(points_x, points_y_a, 'b--')
plt.plot(points_x, points_y_a, 'bo')
red_line = mpatches.Patch(color='red', label='theta value')
blue_line = mpatches.Patch(color='blue', label='analytic value')
plt.legend(handles=[red_line, blue_line])
plt.grid(True)
plt.show()
