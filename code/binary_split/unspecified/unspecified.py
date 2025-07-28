#!/usr/bin/env python
"""
Attempts to remove false positives for woody due to cropping
"""
import sys
import numpy
from rios import applier
from rios import rat
from rios import cuiprogress
from osgeo import gdal

if len(sys.argv) != 3:
    print("usage: woody_cl unspec")
    sys.exit()

# Set up input filename 
infiles = applier.FilenameAssociations()
infiles.woody_cl = sys.argv[1]

# Set up output filename
outfiles = applier.FilenameAssociations()
outfiles.unspec = sys.argv[2]

controls = applier.ApplierControls()
controls.setReferenceImage(infiles.woody_cl) 
controls.resampleMethod='near'
controls.thematic = True
controls.drivername='KEA'
controls.calcStats = True
controls.progress = cuiprogress.CUIProgressBar()

# set up the column as an 'other' input
otherinputs = applier.OtherInputs()
otherinputs.original  = rat.readColumn(infiles.woody_cl, "Original Value")
otherinputs.histogram = rat.readColumn(infiles.woody_cl, "Histogram")
otherinputs.perimeter = rat.readColumn(infiles.woody_cl, "Perimeter")

# Define the applier function
def recodeValid(info, inputs, outputs):

    unspec = otherinputs.original[inputs.woody_cl[0]]

    perimeter = otherinputs.perimeter[inputs.woody_cl[0]]
    histogram = otherinputs.histogram[inputs.woody_cl[0]]

    # centre/edge ratio
    ce_ratio = numpy.divide(histogram - perimeter, histogram, out=numpy.zeros_like(perimeter), where=histogram!=0)

    # set unspecified when less than 25% 'pure' pixels (non-edge, eg. 4x4 minumim feature)
    unspec[(unspec == 3) & (ce_ratio < 0.25)] = 9
    
    # Expand the output array to include a single
    # image band and set as the output dataset.
    outputs.unspec = numpy.expand_dims(unspec, axis=0)

# Apply the recodeRules function.
applier.apply(recodeValid, infiles, outfiles, controls=controls)

