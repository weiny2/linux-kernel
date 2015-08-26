#!/usr/bin/python

# Compare some number of perf test files
#
# Author: Denny Dalessandro (dennis.dalessandro@intel.com)
#

import sys
import re
import os
import numpy
import pylab as pl

from optparse import OptionParser

def main():

    parser = OptionParser()

    parser.add_option("--x", dest="xparm",
                      help="Column index of data for X-axis (1 based)",
                      default=1)
    parser.add_option("--y", dest="yparm",
                      help="Column index of data for Y-axis (1 based)",
                      default=4)
    parser.add_option("--files", dest="flist",
                      help="Comma separted list of file:axis-label")
    parser.add_option("--title", dest="title",
                      help="Title to give the plot")
    parser.add_option("--xscale", dest="xscale",
                      help="Scale for X-axis. Valid values are: log2, log10, <None>")
    parser.add_option("--yscale", dest="yscale",
                      help="Scale for Y-axis. Valid values are: log2, log10, <None>")
    parser.add_option("--xrange", dest="xrange",
                      help="Set x range in the format of min:max"),
    parser.add_option("--yrange", dest="yrange",
                      help="Set y range in the format of min:max"),
    parser.add_option("--xlabel", dest="xlabel",
                      help="Label for X axis, default matches --x of the first file")
    parser.add_option("--ylabel", dest="ylabel",
                      help="Label for Y axis, defualt matches --y of the first file")
    parser.add_option("--output", dest="output",
                      help="Create a PNG file (extension appended automatically)")

    (options, args) = parser.parse_args()

    if not options.flist:
        print "Need to specify a list of files and labels"
        sys.exit(1)

    farray = options.flist.split(",")

    print "X data comes from column", options.xparm
    print "Y data comes from column", options.yparm
    print "Plotting:"
    titles=[]
    xlabel=""
    ylabel=""
    for file in farray:
        (fname, label) = file.split(":", 1)
        if not os.path.exists(fname):
            print fname, "is an invalid file"
            sys.exit(1)
        print fname, "as", label
        titles.append(fname)
        with open(fname) as f:
            lines = f.readlines()
        x_data = []
        y_data = []
        found_data = 0
        for line in lines:
            if found_data:
                match = re.search('^\s*(\S+)\s+(\S+)\s+(\S)+\s+(\S+)\s+(\S+)', line)
                if match:
                    #print line.strip()
                    x_data.append(match.group(int(options.xparm)))
                    y_data.append(match.group(int(options.yparm)))
            elif "#bytes" in line:
                found_data = 1
                match = re.search('^\s*(\S+)\s+(\S+)\s+(\S)+\s+(\S+)\s+(\S+)', line)
                if match:
                    if xlabel == "":
                        xlabel = match.group(int(options.xparm))
                    if ylabel == "":
                        ylabel = match.group(int(options.yparm))

        pl.plot(x_data, y_data, label="%s" % label)
        print ""

    if options.xlabel:
        pl.xlabel(options.xlabel)
    else:
        pl.xlabel(xlabel)

    if options.ylabel:
        pl.ylabel(options.ylabel)
    else:
        pl.ylabel(ylabel)

    pl.legend(loc='best')

    # Set log scale for the axes
    if options.xscale == "log2":
        pl.semilogx(basex=2)

    if options.xscale == "log10":
        pl.semilogx(basex=10)

    if options.yscale == "log2":
        pl.semilogy(basey=2)

    if options.yscale == "log10":
        pl.semilogy(basey=10)

    if options.xrange:
        (min, max) = options.xrange.split(":")
        pl.xlim(int(min),int(max))

    if options.yrange:
        (min, max) = options.yrange.split(":")
        pl.ylim(int(min),int(max))

    # build the title
    title_str = ""
    if not options.title:
        for title in titles:
            if title_str == "":
                title_str = title
            else:
                title_str = title_str + " vs " + title
    else:
        title_str = options.title

    pl.title(title_str)

    if options.output:
        pl.savefig(options.output + ".png")
    else:
        pl.show()

    # TODO (maybe):
    # Get rid of exponential notation and label the axis better
    #   can do later, the axis is fine for analysis we are not publishing these
    #   plots.

if __name__ == "__main__":
    main()

