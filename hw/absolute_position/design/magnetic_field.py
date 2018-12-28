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

import matplotlib.pyplot as plt
import numpy as np
import scipy.integrate

dblquad = scipy.integrate.dblquad
tplquad = scipy.integrate.tplquad

# Magnetic permeability of air
u0 = 1.256637e-6


class Magnet:
    '''Bar magnet field equations from:
    http://demonstrations.wolfram.com/MagneticFieldOfACylindricalBarMagnet/
    '''
    def __init__(self, length=0.003175, radius=0.003175,
                 magnetic_moment_per_volume=0.0968*1e9):

        # Magnetization
        # N35 magnets have a diple moment of 0.0968 A*m^2 according to K&J
        #
        # I honestly have no idea why I need the 1e9 here, and it isn't clear
        # I get results that are comparable with it.  But they have the right
        # shape and about the right magnitude, which is enough for my
        # purposes.
        self.M = magnetic_moment_per_volume

        self.L = length

        # Radius of disc (m)
        self.a = radius

    def b(self, z, p):
        '''Return the scalar magnetic field.'''

        L = self.L
        a = self.a
        M = self.M

        if (abs(z) <= (1.02 * L / 2.0) and abs(p) <= 1.02 * a):
            return 0.0

        def inner(R, phi, Z):
            return ((z - Z) * R /
                    (p * p - 2 * R * p * np.cos(phi) +
                     R * R + (z - Z) ** 2) ** 1.5)

        result, error = tplquad(
            inner,
            -L/2.0, L/2.0,
            lambda q: 0, lambda q: 2 * np.pi,
            lambda q, q2: 0, lambda q, q2: a)

        return (u0 / (4 * np.pi)) * M * result

    def bz(self, z, p):
        '''Return the component of the magnetic field in the Z direction for a
        disc bar magnet.'''

        L = self.L
        a = self.a
        M = self.M

        if (abs(z) <= (1.02 * L / 2.0) and abs(p) <= 1.02 * a):
            return 0.0

        def inner(R, phi):
            return ((R * (L / 2.0 - z) /
                     ((R * R + (L / 2.0 - z) ** 2 + p ** 2 -
                       2.0 * R * p * np.cos(phi)) ** 1.5)) +
                    (R * (L / 2.0 + z) /
                     ((R * R + (L / 2.0 + z) ** 2 + p ** 2 -
                       2 * R * p * np.cos(phi)) ** 1.5)))

        result, error = dblquad(
            inner,
            0, 2.0 * np.pi,
            lambda q: 0.0,
            lambda q: a)

        return (-u0 / (4 * np.pi)) * M * result

    def bp(self, z, p):
        '''Return the component of the magnetic field in the p direction for a
        disc bar magnet.'''

        L = self.L
        a = self.a
        M = self.M

        if (abs(z) <= (1.02 * L / 2.0) and abs(p) <= 1.02 * a):
            return 0.0

        def inner(R, phi):
            numerator = R * (2.0 * p - 2.0 * R * np.cos(phi))
            return (-numerator /
                    (2.0 * (R * R + (L / 2.0 - z) ** 2 + p ** 2 -
                            2.0 * R * p * np.cos(phi)) ** 1.5) +
                    numerator /
                    (2.0 * (R * R + (L / 2.0 + z) ** 2 + p ** 2 -
                            2.0 * R * p * np.cos(phi)) ** 1.5))

        result, error = dblquad(
            inner,
            0, 2.0 * np.pi,
            lambda q: 0, lambda q: a)

        return (-u0 / (4.0 * np.pi)) * M * result


def make_levels(levels):
    result = []
    result += levels
    for x in levels:
        result.append(-x)

    return sorted(result)


def main():
    grid_scale = 0.005
    size = 0.05
    X = np.arange(-size, size, grid_scale)
    Y = np.arange(-size, size, grid_scale)
    U, V = np.meshgrid(X, Y)

    fig, ax = plt.subplots()

    m = Magnet()

    if False:
        Z1 = np.fromiter(map(m.bz, U.ravel(), V.ravel()),
                         np.float).reshape(U.shape)
        Z2 = np.fromiter(map(m.bp, U.ravel(), V.ravel()),
                         np.float).reshape(U.shape)

        N = np.hypot(Z1, Z2)
        q = ax.quiver(X, Y, Z1 / N, Z2 / N, scale=1.0 / grid_scale,
                      units='xy', pivot='middle')
    else:

        B = 10000 * np.fromiter(map(m.b, U.ravel(), V.ravel()),
                                np.float).reshape(U.shape)
        plt.pcolormesh(X, Y, B)
        plt.colorbar()

        cs = plt.contour(X, Y, B,
                         levels=make_levels([2, 5, 10, 20, 50,
                                             100, 500, 1000, 2000]),
                         colors=['black'])
        plt.clabel(cs, inline=False, use_clabeltext=True, fontsize=7)

    plt.show()


if __name__ == '__main__':
    main()
