#!/usr/bin/python
#
# Android unlock pattern combinations script
# Copyright (c) 2012 Zoltan Puskas
# All rights reserved.
#
# This program is free software and redistributred under the 3-clause BSD
# license. For details see attached license file COPYING
#
# Maintainer: Zoltan Puskas <zoltan@sinustrom.info>
# Created on: 2012.02.03.
#

import os

# conbination generator function
def combination_iter(elements, length):
    for i in xrange(len(elements)):
        if length == 1:
            yield (elements[i],)
        else:
            for next in combination_iter(elements[i+1:len(elements)], length-1):
                yield (elements[i],) + next

# function get combinations for a set of elements				
def combination(l, k):
    return list(combination_iter(l, k))

# for all valid ranges of points calculate dot choices
for num in range(4, 10):
	print "If choosing %i dots out of 9 the number of different choices is %i " % (num, len(combination(range(1, 10), num)))
	
