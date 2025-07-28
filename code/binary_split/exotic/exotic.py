#!/usr/bin/env python
"""
Burn exotic forest from DL prediction
"""
import sys
import numpy
from rios import applier
from rios import rat
from rios import cuiprogress
from osgeo import gdal

if len(sys.argv) != 5:
    print("usage: woody dl_pred steep exotic")
    sys.exit()

# Set up input filename 
infiles = applier.FilenameAssociations()
infiles.woody = sys.argv[1]
infiles.dl_pred = sys.argv[2]
infiles.steep = sys.argv[3]

# Set up output filename
outfiles = applier.FilenameAssociations()
outfiles.exotic = sys.argv[4]

controls = applier.ApplierControls()
controls.setReferenceImage(infiles.woody) 
controls.resampleMethod='near'
controls.thematic = True
controls.drivername='KEA'
controls.calcStats = True
controls.progress = cuiprogress.CUIProgressBar()

# Define the applier function
def recodeValid(info, inputs, outputs):

    exotic = numpy.zeros_like(inputs.woody[0],dtype=numpy.uint8)
    exotic = inputs.woody[0]
    
    # exotic forest (limit to woody areas)
    exotic[(inputs.woody[0] == 3) & (inputs.dl_pred[0] == 11)] = 11


    # deciduous hardwoods (limit to lowlands)
    exotic[(inputs.steep[0] == 1) & (inputs.dl_pred[0] == 12)] = 12
    
    # Expand the output array to include a single
    # image band and set as the output dataset.
    outputs.exotic = numpy.expand_dims(exotic, axis=0)

# Apply the recodeRules function.
applier.apply(recodeValid, infiles, outfiles, controls=controls)

