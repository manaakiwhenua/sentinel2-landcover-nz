#!/usr/bin/env python
"""
Burn wetlands, orchards and vineyards from lcdb
"""
import sys
import numpy
from rios import applier
from rios import rat
from rios import cuiprogress
from osgeo import gdal

if len(sys.argv) != 4:
    print("usage: woody lcdb orchards")
    sys.exit()

# Set up input filename 
infiles = applier.FilenameAssociations()
infiles.woody = sys.argv[1]
infiles.lcdb = sys.argv[2]

# Set up output filename
outfiles = applier.FilenameAssociations()
outfiles.wetlands = sys.argv[3]

controls = applier.ApplierControls()
controls.setReferenceImage(infiles.woody) 
controls.resampleMethod='near'
controls.thematic = True
controls.drivername='KEA'
controls.calcStats = True
controls.progress = cuiprogress.CUIProgressBar()

# Define the applier function
def recodeValid(info, inputs, outputs):

    wetlands = numpy.zeros_like(inputs.woody[0],dtype=numpy.uint8)
    wetlands = inputs.woody[0]

    # wetlands
    wetlands[(inputs.lcdb[0] == 45) | (inputs.lcdb[0] == 46)] = 15

    # orchards and vineyards
    wetlands[inputs.lcdb[0] == 33] = 16
    
    # Expand the output array to include a single
    # image band and set as the output dataset.
    outputs.wetlands = numpy.expand_dims(wetlands, axis=0)

# Apply the recodeRules function.
applier.apply(recodeValid, infiles, outfiles, controls=controls)

