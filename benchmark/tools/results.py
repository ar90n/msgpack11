#!/usr/bin/env python3

# forked from https://github.com/ludocode/schemaless-benchmarks/blob/master/tools/results.py

import csv, sys
import collections
from functools import reduce
from math import sqrt

csvname = 'results.csv'
NAME, VERSION, OBJECT_SIZE, TIME, BINARY_SIZE, HASH = range(6)

# collect data in csv
results = {}
with open(csvname) as csvfile:
    reader = csv.reader(csvfile)
    for row in reader:
        name = row[NAME].split('/')[-1]
        if not name  in results:
            results[name] = {}
            results[name]['size'] = int(row[BINARY_SIZE])
            results[name]['version'] = row[VERSION]
            results[name]['hash'] = row[HASH]
            results[name]['time'] = collections.defaultdict(list)

        object_size = int(row[OBJECT_SIZE])
        results[name]['time'][object_size].append( float(row[TIME]) )

def print_header():
    header = '| Library | Binary size |'
    divider = '|----|----|'
    for s in ['Smallest', 'Small', 'Medium', 'Large', 'Largest']:
        header += ' time[ms] @ %s |' % s
        divider += '----|'
    print(header)
    print(divider)

def print_table(results):
    print_header()
    for name,values in results.items():
        if name in ['hash-data', 'hash-object']:
            continue
        hash_name = (name[-6:] == 'unpack') and 'hash-object' or 'hash-data'

        row = '| %s(v%s) |' % (name, values['version'])

        size = results[name]['size'] - results[hash_name]['size']
        row += ' %s |' % str(size)

        for i in range(1,6):
            timestr = rowtime_str(results[name]['time'][i], results[hash_name]['time'][i])
            row += ' %s |' % str(timestr)

        print(row)

def rowtime(row):
    # copy list
    times = list(row)

    # drop highest and lowest
    if len(times) > 6:
        times.remove(max(times))
        times.remove(min(times))

    # calculate mean and stdev
    count = len(times)
    mean = sum(times) / count
    if count > 1:
        sumsqr = reduce(lambda x, y: x + pow(y - mean, 2), times, 0)
        stdev = sqrt(sumsqr / (count - 1))
    else:
        stdev = mean

    return mean, stdev

def rowstring(net, stdev):
    return '%.2f' % net

def rowtime_str(row, sub):
    time, timedev = rowtime(row)
    subtime, subdev = rowtime(sub)
    net = time - subtime
    stdev = sqrt(pow(timedev, 2) + pow(subdev, 2))
    return rowstring(net, stdev)

print_table(results)
