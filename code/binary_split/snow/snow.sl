#!/bin/bash
#SBATCH -A landcare00034
#SBATCH --job-name=snow    # job name (shows up in the queue)
#SBATCH -o snow.job-%j.out
#SBATCH -e snow.job-%j.err
#SBATCH --time=02:00:00        # Walltime (HH:MM:SS)
#SBATCH --mem=100G               # memory (was 100G)
#SBATCH --ntasks=1             # number of tasks (e.g. MPI)
#SBATCH --cpus-per-task=1      # number of cores per task (e.g. OpenMP)
#SBATCH --partition=hugemem      # specify a partition (was hugmem)
#SBATCH --hint=nomultithread   # don't use hyperthreading
#SBATCH --mail-type=FAIL
#SBATCH --mail-user=shepherdj@landcareresearch.co.nz

if [ "$#" -ne 3 ]; then
    echo "expecting three arguments 'flats' [nz|north|south|stewart|chathams] 'snow'"
    exit -1
fi

cdir="${HOME}/code/gdal_c_software/ecosat_woody"

flats="$1"
island="$2"
snow="$3"

if [ "$island" == 'chathams' ] || [ "$island" == 'ci' ]
then
    dem="/nesi/project/landcare00034/raster/dem/linz/dem_linz_ci_citm_10m.kea"
    mask="/nesi/project/landcare00034/raster/masks/ci_citm_land_buf.kea"
elif [ "$island" == 'north' ]
then
    dem="/nesi/project/landcare00034/raster/dem/linz/dem_linz_ni_nztm_10m.kea"
    mask="/nesi/project/landcare00034/raster/masks/north_island.kea"
elif [ "$island" == 'south' ]
then
    dem="/nesi/project/landcare00034/raster/dem/linz/dem_linz_si_nztm_10m.kea"
    mask="/nesi/project/landcare00034/raster/masks/south_island.kea"
elif [ "$island" == 'stewart' ]
then
    dem="/nesi/project/landcare00034/raster/dem/linz/dem_linz_st_nztm_10m.kea"
    mask="/nesi/project/landcare00034/raster/masks/stewart_island.kea"
elif [ "$island" == 'three_kings' ]
then
    dem="/nesi/project/landcare00034/raster/dem/linz/dem_linz_tk_nztm_10m.kea"
    mask="/nesi/project/landcare00034/raster/masks/three_kings_island.kea"
fi

export GDAL_CACHEMAX=256

if [ -e "${snow}" ]; then
    echo "File ${snow} already exists"
else 
    ${cdir}/snow/snow.py ${flats} ${dem} ${mask} ${snow}
fi
