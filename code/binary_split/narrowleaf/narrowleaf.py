#!/usr/bin/env python
"""
Classify Narrow-leaved Scrub
"""
import sys
import numpy
from rios import applier
from rios import rat
from rios import cuiprogress
from osgeo import gdal

if len(sys.argv) != 4:
    print("usage: woody flats narrow")
    sys.exit()

# Set up input filename 
infiles = applier.FilenameAssociations()
infiles.woody = sys.argv[1]
infiles.flats = sys.argv[2]

# Set up output filename
outfiles = applier.FilenameAssociations()
outfiles.narrow = sys.argv[3]

controls = applier.ApplierControls()
controls.setReferenceImage(infiles.woody) 
controls.resampleMethod='near'
controls.thematic = True
controls.drivername='KEA'
controls.calcStats = True
controls.progress = cuiprogress.CUIProgressBar()

# Define the applier function
def recodeValid(info, inputs, outputs):

    narrow = numpy.zeros_like(inputs.woody[0],dtype=numpy.uint8)

    nir  = numpy.array(inputs.flats[6], dtype=float)
    swir = numpy.array(inputs.flats[8], dtype=float)

    delta = nir - swir
    total = nir + swir
    
    index = numpy.divide(delta, total, out=numpy.zeros_like(delta), where=total!=0)

    narrow = inputs.woody[0]

#    narrow[(inputs.woody[0] == 3) & (index < 0.33)] = 10
    narrow[(inputs.woody[0] == 3) & (index < 0.25)] = 10
    
    # Expand the output array to include a single
    # image band and set as the output dataset.
    outputs.narrow = numpy.expand_dims(narrow, axis=0)

# Apply the recodeRules function.
applier.apply(recodeValid, infiles, outfiles, controls=controls)

