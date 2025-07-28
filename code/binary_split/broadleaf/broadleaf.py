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
    print("usage: narrow flats broad")
    sys.exit()

# Set up input filename 
infiles = applier.FilenameAssociations()
infiles.narrow = sys.argv[1]
infiles.flats = sys.argv[2]

# Set up output filename
outfiles = applier.FilenameAssociations()
outfiles.broad = sys.argv[3]

controls = applier.ApplierControls()
controls.setReferenceImage(infiles.narrow) 
controls.resampleMethod='near'
controls.thematic = True
controls.drivername='KEA'
controls.calcStats = True
controls.progress = cuiprogress.CUIProgressBar()

# Define the applier function
def recodeValid(info, inputs, outputs):

    broad = numpy.zeros_like(inputs.narrow[0],dtype=numpy.uint8)

    green = numpy.array(inputs.flats[1], dtype=float)
    nir   = numpy.array(inputs.flats[6], dtype=float)
    swir  = numpy.array(inputs.flats[8], dtype=float)

    broad = inputs.narrow[0]

    broad[(inputs.narrow[0] == 3) & (green > 40)] = 13
    
    # Expand the output array to include a single
    # image band and set as the output dataset.
    outputs.broad = numpy.expand_dims(broad, axis=0)

# Apply the recodeRules function.
applier.apply(recodeValid, infiles, outfiles, controls=controls)
