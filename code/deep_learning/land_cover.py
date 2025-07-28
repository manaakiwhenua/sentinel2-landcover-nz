import datetime
import shutil
import numpy as np
import os
import rasterio
from rasterio import windows
from tqdm import tqdm
from keras.models import load_model as keras_load_model


def image_prep(batch, mean_pixel, stdev_pixel, nodata, num_bands, normalise_tiles=False):
    """
    Normalises the imagery:
        - scale each band to even out their spread, applying the same stretch to all tiles
        - if normalise_tiles=true, further stretch *all bands* of each tile to be mean=0, stdev=1
    """
    for band in range(num_bands):
        batch[..., band] -= mean_pixel[band]
        if stdev_pixel[band] > 0:
            batch[..., band] /= stdev_pixel[band]

    if normalise_tiles:
        for image in range(len(batch)):
            image_data = batch[image, :, :, :]
            img_mean = np.mean(image_data)
            img_stdev = np.std(image_data)
            if img_stdev ==0: img_stdev = 1
            batch[image, :, :, :] -= img_mean
            batch[image, :, :, :] /= img_stdev        
    
    batch[np.isnan(batch)] = nodata
    return batch


def create_image_tile(tile_size, image_file_paths, window, geometry=None):
    """
    Extract the data for this tile from the input image/stack
    """
    ref_path = image_file_paths[0]
    ref_image = rasterio.open(ref_path, "r")

    if window is None:  # create the window from the supplied geometry
        window = rasterio.features.geometry_window(ref_image, [geometry])

    left, bottom, right, top = rasterio.windows.bounds(window, ref_image.transform)

    image = None

    for i, image_file_path in enumerate(image_file_paths):
        image_file = rasterio.open(image_file_path, "r")

        window = rasterio.windows.from_bounds(
            left, bottom, right, top, image_file.transform
        )
        
        out_shape = (image_file.count, tile_size, tile_size)

        if (
            abs(window.height - tile_size) > 1 or abs(window.width - tile_size) > 1):
            im = image_file.read(
                window=window, out_shape=out_shape, resampling=Resampling.bilinear
            )
        else:
            im = image_file.read(window=window)  # Don't resize

        im[im < 0.0] = 0.0

        im = np.moveaxis(ensure_dims(tile_size, im), 0, -1).astype(np.float32)  # Check type!

        if image is None:
            image = im
        else:
            image = np.append(image, im, axis=-1)
    return image

    
def ensure_dims(tile_size, raster):
    """
    Pad any clipped tiles.
    """
    new_raster = np.zeros((raster.shape[0], tile_size, tile_size))
    _, nrows, ncols = raster.shape
    nrows_difference = nrows - tile_size
    ncols_difference = ncols - tile_size
    max_row = nrows - nrows_difference if nrows_difference > 0 else nrows
    max_col = ncols - ncols_difference if ncols_difference > 0 else ncols
    new_raster[:, :max_row, :max_col] = raster[:, :max_row, :max_col]
    return new_raster
    

def segmentation_prediction_to_raster(batch, probs, results, batch_windows, dst, tile_size, overlap):
    """
    Stitch the results back together into a geolocated raster.
    """
    overlap_half = int(overlap / 2)

    for i in range(batch.shape[0]):

        if overlap_half > 0.0:
            result = results[
                i,
                overlap_half : -overlap_half,
                overlap_half : -overlap_half,
                :,
            ]
        else:
            result = results[i]  # no overlap

        window = windows.Window(
            col_off=batch_windows[i].col_off + overlap_half,
            row_off=batch_windows[i].row_off + overlap_half,
            width=tile_size - overlap,
            height=tile_size - overlap
        )

        try:
            dst.write(result[:, :, 0], 1, window=window)
        except:
            pass  # catch out of range errors


def predict_land_cover(predict_file_paths, 
                    model_path, 
                    out_path, 
                    num_classes, 
                    mean_pixel, 
                    stdev_pixel, 
                    num_bands=10, 
                    tile_size=512, 
                    nodata=0, 
                    overlap=0, 
                    normalise_tiles=False):
    """
    Run the deep learning model over the provided images/stacks of images.
    """

    model = keras_load_model(model_path, compile=False)

    for predict_files in predict_file_paths:
        # Handle single image or a stack of multiple
        if not isinstance(predict_files, list):
            predict_files = [predict_files]
        predict_file = predict_files[0]
        predict_file_name, _ = os.path.splitext(os.path.basename(predict_file))

        prediction_raster_out = os.path.join(out_path, f"prediction_{predict_file_name}.png")

        with rasterio.Env(GDAL_CACHEMAX=512 * 14):
            src = rasterio.open(predict_file)
            kwargs = src.meta
            ncols, nrows = src.meta["width"], src.meta["height"]
            kwargs["dtype"] = "uint8"
            kwargs["nodata"] = nodata
            kwargs["driver"] = "PNG"
            kwargs["count"] = 1

            dst = rasterio.open(prediction_raster_out, "w", **kwargs)

            instance_id = 0
            tile_id = 0
            instance_polys = []
            instance_infos = []
            tiles_tly = np.arange(0, nrows, tile_size - overlap)
            tiles_tlx = np.arange(0, ncols, tile_size - overlap)
            batch = []
            batch_windows = []
            nbatch = 0
            start_time = datetime.datetime.now()

            print(f"\nStarting the prediction for {predict_file}...", flush=True)

            max_ntiles = len(tiles_tly) * len(tiles_tlx)
            pbar = tqdm(total=max_ntiles)
            for tile_tly in tiles_tly:
                for tile_tlx in tiles_tlx:
                    tile_id += 1

                    pbar.update(1)
                    pbar.refresh()

                    # READ SINGLE TILE DATA
                    window = windows.Window(col_off=tile_tlx, row_off=tile_tly, width=tile_size, height=tile_size)

                    tiledata = create_image_tile(tile_size, predict_files, window=window)
                    # convert to numpy array
                    tile_nrows, tile_ncols, nbands = tiledata.shape
                    tile = np.zeros((tile_size, tile_size, nbands), dtype=tiledata.dtype)
                    tile[:tile_nrows, :tile_ncols, :] = tiledata

                    if tile.max():
                        nbatch += 1
                        
                        batch.append(tile)

                        tile_window = windows.Window(col_off=tile_tlx, row_off=tile_tly, width=tile_ncols, height=tile_nrows)
                        batch_windows.append(tile_window)

                        batch = np.stack(batch, axis=0)

                        # PREDICT SEMANTIC LABELS
                        batch = batch.astype(np.float32)
                        batch = image_prep(batch, mean_pixel, stdev_pixel, nodata, nbands, normalise_tiles)

                        if batch.ndim <= 3:
                            batch = batch[np.newaxis, :, :, :]
                        probs = model.predict(batch, verbose=0)
                        results = np.zeros((probs.shape[0], tile_size, tile_size, num_classes + 1), dtype=np.float32)
                        for i in range(probs.shape[0]):
                            results[i, ..., 0] = np.argmax(
                                probs[i, ...], axis=2
                            )
                            results[i, ..., 1:] = probs[i]

                        segmentation_prediction_to_raster(
                            batch,
                            probs,
                            results,
                            batch_windows,
                            dst,
                            tile_size,
                            overlap)

                        nbatch += 1
                        tile = None
                        tiledata = None
                        batch = []
                        batch_windows = []
                        instance_polys = []
                        tile_id += 1
                    else:
                        pass

            src.close()

        pbar.close()
        print(f"Prediction time: {datetime.datetime.now() - start_time}")
        dst.close()


def main():

    overlap = 64
    num_classes = 14
    tile_size = 512
    nodata = -1
    num_bands = 10

    predict_folder = "predictions/"

    # ----------------------------------------------------
    # Model and settings for the North Island (multi-year)
    # ----------------------------------------------------
    model_path = "models/NI_multi_epoch_421_val_loss-0.1696.h5"
    mean_pixel =  [22.389, 41.368, 31.233, 77.067, 201.944, 236.537, 264.244, 288.621, 146.476, 67.495]
    stdev_pixel = [17.783, 21.654, 25.856, 47.374,  74.454,  89.253,  99.987, 105.057,  65.281, 41.060]
    normalise_tiles = False
    predict_files = ["samples/NI_s2_2324_sample_small.tif"]

    """
    # ---------------------------------------
    # Model and settings for the South Island
    # ---------------------------------------
    model_path = "models/SI_1819_epoch_818_val_loss-0.1736.h5"
    mean_pixel =  [37.425, 54.802, 51.401, 95.788, 202.041, 227.912, 247.493, 266.299, 159.103, 85.660]
    stdev_pixel = [47.362, 48.887, 54.713, 56.164,  87.026, 100.144, 104.132, 108.311,  73.400, 53.479]
    normalise_tiles = True
    predict_files = ["samples/SI_s2_2324_sample_small.tif"]
    """
    
    predict_land_cover(predict_files, model_path, predict_folder, num_classes, mean_pixel, stdev_pixel, num_bands, tile_size, nodata, overlap, normalise_tiles)

if __name__ == "__main__":
    main()