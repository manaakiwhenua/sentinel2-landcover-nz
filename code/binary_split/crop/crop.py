#!/usr/bin/env python
"""
Classify Cropping
"""
import sys
import numpy
from rios import applier
from rios import rat
from rios import cuiprogress
from osgeo import gdal

if len(sys.argv) != 6:
    print("usage: exotic ndvi steep dl_pred crop")
    sys.exit()

# Set up input filename 
infiles = applier.FilenameAssociations()
infiles.exotic  = sys.argv[1]
infiles.ndvi    = sys.argv[2]
infiles.steep   = sys.argv[3]
infiles.dl_pred = sys.argv[4]

# Set up output filename
outfiles = applier.FilenameAssociations()
outfiles.crop = sys.argv[5]

controls = applier.ApplierControls()
controls.setReferenceImage(infiles.exotic) 
controls.resampleMethod='near'
controls.thematic = True
controls.drivername='KEA'
controls.calcStats = True
controls.progress = cuiprogress.CUIProgressBar()

# Define the applier function
def recodeValid(info, inputs, outputs):

    crop = numpy.zeros_like(inputs.exotic[0],dtype=numpy.uint8)

    crop = inputs.exotic[0]

    flat = inputs.steep[0] == 1
    grass = inputs.dl_pred[0] == 6
    annual = inputs.dl_pred[0] == 5
    perennial = inputs.dl_pred[0] == 4

    potential = numpy.any([grass, annual, perennial], axis=0)

    prop_veg = inputs.ndvi[4]
    ndvi_all = inputs.ndvi[0]
    ndvi_veg = inputs.ndvi[2]
    
    crop[(flat) & (potential) & (prop_veg < 6000) & (ndvi_veg > 3000) & (ndvi_all < 0)] = 14

    # Expand the output array to include a single
    # image band and set as the output dataset.
    outputs.crop = numpy.expand_dims(crop, axis=0)

# Apply the recodeRules function.
applier.apply(recodeValid, infiles, outfiles, controls=controls)

