#!/bin/bash
#SBATCH -A landcare00034
#SBATCH --job-name=run_blc    # job name (shows up in the queue)
#SBATCH -o run_blc.job-%j.out
#SBATCH -e run_blc.job-%j.err
#SBATCH --time=03:00:00           # Walltime (HH:MM:SS)
#SBATCH --mem=10G                # memory
#SBATCH --ntasks=1                # number of tasks (e.g. MPI)
#SBATCH --cpus-per-task=1         # number of cores per task (e.g. OpenMP)
#SBATCH --hint=nomultithread      # don't use hyperthreading
#SBATCH --mail-type=FAIL
#SBATCH --mail-user=shepherdj@landcareresearch.co.nz

if [ "$#" -ne 3 ]; then
    echo "expecting three arguments [north|south|stewart] [2324|2425] [v1|v2]"
    exit -1
fi

island="${1}"
mosaic="${2}"
version="${3}"

wdir="/nesi/project/landcare00035/sen2_mosaics/mosaic_S2_${mosaic}_${version}"
cdir="${HOME}/code/gdal_c_software/ecosat_woody"
mdir="/nesi/project/landcare00034/raster/masks"
odir="/nesi/project/landcare00004/blc"

if [ ! -f ${odir}/${island}_${mosaic}_snow.kea ]
then
    ${cdir}/snow/snow.sl \
	   ${wdir}/${island}_sen2_flats_${mosaic}.kea \
	   ${island} \
	   ${odir}/${island}_${mosaic}_snow.kea
fi

if [ ! -f ${odir}/${island}_${mosaic}_water.kea ]
then
    ${cdir}/water_ndvi/water_ndvi \
	   ${wdir}/${island}_sen2_flats_${mosaic}.kea \
	   ${mdir}/${island}_island.kea \
	   ${odir}/${island}_${mosaic}_snow.kea \
	   ${odir}/${island}_${mosaic}_water.kea
fi

if [ ! -f ${odir}/${island}_${mosaic}_woody.kea ]
then
    ${cdir}/woody/woody \
	   ${wdir}/${island}_sen2_flats_${mosaic}.kea \
	   ${odir}/${island}_${mosaic}_water.kea \
	   ${mdir}/nzlcdb50_2018.kea \
	   ${odir}/${island}_${mosaic}_woody.kea
fi

# ${odir}/${island}_${mosaic}_woody_cl.kea is raster clumping of the previous woody layer

if [ -f ${odir}/${island}_${mosaic}_woody_cl.kea ] && [ ! -f ${odir}/${island}_${mosaic}_unspec.kea ]
then
    ${cdir}/unspecified/unspecified.py \
           ${odir}/${island}_${mosaic}_woody_cl.kea \
           ${odir}/${island}_${mosaic}_unspec.kea

    ${cdir}/add_blc_ct.py \
	   ${odir}/${island}_${mosaic}_unspec.kea
fi

if [ -f ${odir}/${island}_${mosaic}_unspec.kea ] && [ ! -f ${odir}/${island}_${mosaic}_exotic.kea ]
then
    ${cdir}/exotic/exotic.py \
           ${odir}/${island}_${mosaic}_unspec.kea \
           ${odir}/nz_dl_prediction.kea \
           ${odir}/nzsteepflat.kea \
           ${odir}/${island}_${mosaic}_exotic.kea

    ${cdir}/add_blc_ct.py \
           ${odir}/${island}_${mosaic}_exotic.kea
fi

if [ -f ${odir}/${island}_${mosaic}_exotic.kea ] && [ ! -f ${odir}/${island}_${mosaic}_narrow.kea ]
then
    ${cdir}/narrowleaf/narrowleaf.py \
           ${odir}/${island}_${mosaic}_exotic.kea \
           ${wdir}/${island}_sen2_flats_${mosaic}.kea \
           ${odir}/${island}_${mosaic}_narrow.kea

    ${cdir}/add_blc_ct.py \
           ${odir}/${island}_${mosaic}_narrow.kea
fi

if [ -f ${odir}/${island}_${mosaic}_narrow.kea ] && [ ! -f ${odir}/${island}_${mosaic}_broad.kea ]
then
    ${cdir}/broadleaf/broadleaf.py \
           ${odir}/${island}_${mosaic}_narrow.kea \
           ${wdir}/${island}_sen2_flats_${mosaic}.kea \
           ${odir}/${island}_${mosaic}_broad.kea

    ${cdir}/add_blc_ct.py \
           ${odir}/${island}_${mosaic}_broad.kea
fi

if [ -f ${odir}/${island}_${mosaic}_broad.kea ] && [ ! -f ${odir}/${island}_${mosaic}_crop.kea ]
then
    ${cdir}/crop/crop.py \
           ${odir}/${island}_${mosaic}_broad.kea \
           ${odir}/nz_median_ndvi.kea \
           ${odir}/nzsteepflat.kea \
           ${odir}/nz_dl_prediction.kea \
           ${odir}/${island}_${mosaic}_crop.kea

    ${cdir}/add_blc_ct.py \
           ${odir}/${island}_${mosaic}_crop.kea
fi
 
if [ -f ${odir}/${island}_${mosaic}_crop.kea ] && [ ! -f ${odir}/${island}_${mosaic}_wetlands.kea ]
then
    ${cdir}/wetlands/wetlands.py \
           ${odir}/${island}_${mosaic}_crop.kea \
           ${odir}/nzlcdb50_2018.kea \
           ${odir}/${island}_${mosaic}_wetlands.kea

    ${cdir}/add_blc_ct.py \
           ${odir}/${island}_${mosaic}_wetlands.kea
fi

# ${odir}/${island}_${mosaic}_wetlands.kea is the final output, rename to ${odir}/${island}_${mosaic}_blc.kea

if [ -f ${odir}/${island}_${mosaic}_wetlands.kea ] && [ ! -f ${odir}/${island}_${mosaic}_blc.kea ]
then
    ln -s ${odir}/${island}_${mosaic}_wetlands.kea ${odir}/${island}_${mosaic}_blc.kea
fi
