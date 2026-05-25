import rasterio
from rasterio.enums import Resampling
import numpy as np
import os

# Define the input file and the output directory
input_tiff = 'raw_terrain.tif'
output_dir = 'data_bins'

if not os.path.exists(output_dir):
    os.makedirs(output_dir)

# These are our specific Level of Detail (LOD) grid sizes.
# We start at 2049x2049 (LOD 0) and halve it down to 33x33 (LOD 6)
# The "+ 1" ensures vertices share borders perfectly in our quadtree.
lod_sizes = [2049, 1025, 513, 257, 129, 65, 33]

print(f"Opening {input_tiff}...")

with rasterio.open(input_tiff) as dataset:
    # Get the NoData value (usually a large negative number like -32768 for oceans/voids)
    nodata_val = dataset.nodata
    
    for lod_level, size in enumerate(lod_sizes):
        print(f"Processing LOD {lod_level} at {size}x{size}...")
        
        # Read the first band of the TIFF, explicitly requesting our target shape.
        # Rasterio automatically handles the resampling (scaling) for us.
        data = dataset.read(
            1, 
            out_shape=(size, size),
            resampling=Resampling.bilinear
        )
        
        # Clean up NoData values by flattening them to 0 (sea level)
        if nodata_val is not None:
            data[data == nodata_val] = 0.0
            
        # SRTM data is usually 16-bit integers. 
        # We cast to 32-bit floats because our C++ engine expects floats to compute quantization.
        data_float32 = data.astype(np.float32)
        
        # Write directly to disk as a raw, contiguous binary block.
        # No headers, no metadata, just pure byte payloads.
        out_filepath = os.path.join(output_dir, f"lod_{lod_level}_{size}x{size}.bin")
        data_float32.tofile(out_filepath)

print("Data extraction complete. Ready for C++ ingestion.")