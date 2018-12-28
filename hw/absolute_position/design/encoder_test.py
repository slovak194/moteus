#!/usr/bin/python3

# Copyright 2018 Josh Pieper, jjp@pobox.com.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

'''Models a custom absolute angle encoder with radially spaced hall
effect sensors on a stator, with a rotor that has opposing magnets at
180 degrees phase shift.

Calibrated against: https://www.kjmagnetics.com/calculator.asp?calcType=disc
'''

import argparse
import math
import matplotlib.collections
import matplotlib.pyplot as plt
from mpl_toolkits import mplot3d
import numpy as np
import random

import magnetic_field


QUANTIZE_MAX = 0.04
ADC_SAMPLE_NOISE = 3


def quantize(value):
    '''Inject quantization noise, assuming a maximum reading of 0.04 and a
    10 bit ADC.'''
    adc_counts = max(
        -512, min(511, int(512 * value / QUANTIZE_MAX) +
                  random.randint(-ADC_SAMPLE_NOISE, ADC_SAMPLE_NOISE)))
    return (adc_counts / 512.0) * QUANTIZE_MAX


class Sensor:
    def __init__(self, theta, radius):
        self.x = radius * np.cos(theta)
        self.y = radius * np.sin(theta)
        self.z = 0.0

    def field(self, rotor):
        '''Measure the magnetic field from magnets in the rotor.'''
        result = 0.0
        for magnet in rotor.magnets:
            result += magnet.field(self.x, self.y, self.z)
        return quantize(-1.0 * result)


class Magnet:
    def __init__(self, orientation, radius, thickness):
        self.orientation = orientation
        self.radius = radius
        self.magnet = magnetic_field.Magnet(length=thickness, radius=radius)
        self.x = 0.
        self.y = 0.
        self.z = 0.

    def set_pos(self, x, y, z):
        self.x = float(x)
        self.y = float(y)
        self.z = float(z)

    def get_verts(self):
        result = []
        for i in range(17):
            theta = 2.0 * np.pi * i / 16.0
            result.append(
                (self.radius * np.cos(theta) + self.x,
                 self.radius * np.sin(theta) + self.y))
        return result

    def field(self, x, y, z):
        dx = x - self.x
        dy = y - self.y
        dz = z - self.z

        field = self.magnet.b(dz, np.hypot(dx, dy))
        return field * self.orientation


class Rotor:
    def __init__(self, z, radius, magnet_radius, magnet_thickness):
        self.z = z
        self.radius = radius
        self.magnet1 = Magnet(orientation=1.0,
                              radius=magnet_radius,
                              thickness=magnet_thickness)
        self.magnet2 = Magnet(orientation=-1.0,
                              radius=magnet_radius,
                              thickness=magnet_thickness)

        self.magnets = [self.magnet1, self.magnet2]

        self.set_theta(0.0)

    def set_theta(self, value):
        self.theta = value

        self.magnet1.set_pos(self.radius * np.cos(self.theta),
                             self.radius * np.sin(self.theta),
                             self.z)
        t2 = self.theta + np.pi
        self.magnet2.set_pos(self.radius * np.cos(t2),
                             self.radius * np.sin(t2),
                             self.z)


def plot3d(sensors, rotor):
    fig = plt.figure()
    ax = plt.axes(projection='3d')

    ax.scatter([x.x for x in sensors],
               [x.y for x in sensors],
               [x.z for x in sensors])

    polys = matplotlib.collections.PolyCollection(
        [x.get_verts() for x in rotor.magnets],
        facecolors=['red', 'green'])
    ax.add_collection3d(polys, zs=[0.01])

    plt.show()


def wrap_neg_pi_to_pi(value):
    return (value + np.pi) % (2 * np.pi) - np.pi


def estimate_theta(sensor_values):
    data = np.array(sensor_values)
    median = np.median(data)
    normalized = list([(b, a) for a, b in enumerate((data - median).tolist())])

    big, small = max(normalized), min(normalized)

    sign = 1.0

    if abs(small[0]) > abs(big[0]):
        sign = -1.0
        normalized = [(a * -1, b) for a, b in normalized]
        big, small = (small, big)

    index = big[1]
    base_result = index * 2.0 * np.pi / len(sensor_values)

    opposite_left = (index + (len(sensor_values) // 2)) % len(sensor_values)
    opposite_right = ((index + ((len(sensor_values) + 1) // 2)) %
                      len(sensor_values))

    left_value = abs(normalized[opposite_left][0])
    right_value = abs(normalized[opposite_right][0])

    offset_sign = 1.0
    if left_value > right_value:
        # We are going to offset to the right.
        offset_sign = -1.0
        left_value, right_value = right_value, left_value

    offset_unit = ((right_value - left_value) /
                   max(8e-5, (abs(big[0]) - left_value)))
    if False:
        # An attempt at a heuristic that made things worse.
        exponent = max(0.2, min(1.0, abs(left_value) /
                                max(8e-5, abs(big[0]) * 10.0)))
    else:
        exponent = 1.0
    offset = (offset_sign * (offset_unit ** exponent) *
              (0.5 * np.pi / len(sensor_values)))

    if False:
        # An attempt at a different heuristic.

        ratio = (normalized[opposite_right][0] /
                 normalized[opposite_left][0])
        offset = ((math.atan(ratio) / np.pi) *
                  (2.0 * np.pi / (len(sensor_values))))

    result = base_result + offset

    if sign < 0:
        result += np.pi

    return wrap_neg_pi_to_pi(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)

    parser.add_argument('--num-sensors', default=5, type=int)
    parser.add_argument('--sensor-radius', default=0.0254, type=float)
    parser.add_argument('--rotor-z', default=0.008, type=float)
    parser.add_argument('--magnet-radius', default=0.003175, type=float)
    parser.add_argument('--magnet-thickness', default=0.003175, type=float)
    parser.add_argument('--plot3d', action='store_true',
                        help='Plot a 3D representation of the sensor geometry')
    parser.add_argument('--angle-increment', default=0.05, type=float)
    parser.add_argument('--plot-estimates', action='store_true')

    args = parser.parse_args()

    # The sensors are placed uniformly around the center.
    sensors = []
    sensor_radius = args.sensor_radius
    num_sensors = args.num_sensors
    for i in range(num_sensors):
        theta = 2.0 * i * np.pi / num_sensors
        sensors.append(Sensor(theta, sensor_radius))

    rotor = Rotor(z=args.rotor_z, radius=sensor_radius,
                  magnet_radius=args.magnet_radius,
                  magnet_thickness=args.magnet_thickness)

    if args.plot3d:
        plot3d(sensors, rotor)
        return

    thetas = []
    fields = [[] for _ in range(len(sensors))]
    estimates = []
    errors = []

    for theta in np.arange(0, 2 * np.pi, args.angle_increment):
        rotor.set_theta(theta)
        thetas.append(theta)
        for i, sensor in enumerate(sensors):
            fields[i].append(sensor.field(rotor))

        estimate = estimate_theta([x[-1] for x in fields])
        estimates.append(math.degrees(estimate))
        errors.append(math.degrees(wrap_neg_pi_to_pi(estimate - theta)))

    first_plot = None

    for i in range(len(sensors)):
        this_plot = plt.subplot(len(sensors) + 1, 1, i + 1,
                                sharex=first_plot, sharey=first_plot)
        if first_plot is None:
            first_plot = this_plot
        plt.plot(thetas, fields[i])
        plt.ylabel('sensor {}'.format(i + 1))

    plt.subplot(len(sensors) + 1, 1, len(sensors) + 1, sharex=first_plot)
    if args.plot_estimates:
        plt.plot(thetas, estimates)
    plt.plot(thetas, errors, label='error')
    plt.legend()
    plt.xlabel('theta (rad)')
    plt.ylabel('estimator (deg)')

    plt.show()


if __name__ == '__main__':
    main()
