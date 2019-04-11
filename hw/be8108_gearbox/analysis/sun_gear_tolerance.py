#!/usr/bin/python3

from matplotlib import pyplot

# The following was pasted from a google sheet.

bore_string = \
'''
7.978
7.967
7.965
7.968
7.968
7.968
7.981
7.977
7.97
7.978
7.967
7.964
7.968
7.981
7.971
7.977
7.972
7.976
7.971
7.974
7.976
7.976
7.973
7.962
7.972
7.988
7.983
7.976
7.967
7.978
7.974
7.956
7.98
7.981
7.978
7.974
'''

bores = [float(x) for x in bore_string.split()]
pyplot.title("Sun gear bore tolerance")
pyplot.hist(bores, rwidth=0.9)
pyplot.show()
