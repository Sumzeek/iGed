import os
import sys
import numpy as np
import logging


@staticmethod
def import_optixbaker():
    """Import the compiled optixbaker extension from OptixBaker/assets."""
    logging.info("[Import] Attempting to import optixbaker")

    ptx_default = os.path.join("OptixBaker", "assets", "optix_kernel.ptx")
    if os.path.isfile(ptx_default):
        os.environ["OPTIX_INTERSECT_PTX"] = ptx_default

    assets_path = os.path.join(os.path.dirname(__file__), "OptixBaker", "assets")
    if os.path.isdir(assets_path) and assets_path not in sys.path:
        sys.path.insert(0, assets_path)
    try:
        import optixbaker
        logging.info("[Import] optixbaker loaded successfully")
        return optixbaker
    except ImportError:
        logging.error(
            "[Import] Failed to import optixbaker. Ensure it is built and that\n"
            "Expected path exists: OptixBaker/assets/optixbaker.pyd",
        )
        raise


if __name__ == "__main__":
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s [%(levelname)s] %(message)s",
        handlers=[logging.StreamHandler(sys.stdout)],
    )

    # Import the extension module
    optixbaker = import_optixbaker()

    # Rays: three test rays
    origins = np.array(
        [
            [0.0, 0.0, 1.0],  # towards +Z triangle
            [0.0, 0.0, -1.0],  # towards -Z triangle
            [1.0, 0.0, 0.0],  # miss
        ],
        dtype=np.float32,
    )
    directions = np.array(
        [
            [0.0, 0.0, 1.0],  # +Z
            [0.0, 0.0, -1.0],  # -Z
            [0.0, 1.0, 0.0],  # +Y (miss)
        ],
        dtype=np.float32,
    )

    # Two triangles: build vertices (Vx3) and indices (Tx3) explicitly
    # tri 0 at z=+5, tri 1 at z=-5
    vertices = np.array(
        [
            [-1.0, -1.0, 5.0], [1.0, -1.0, 5.0], [0.0, 1.0, 5.0],  # tri id 0
            [-1.0, -1.0, -5.0], [1.0, -1.0, -5.0], [0.0, 1.0, -5.0],  # tri id 1
        ],
        dtype=np.float32,
    )
    indices = np.array(
        [
            [0, 1, 2],
            [3, 4, 5],
        ],
        dtype=np.uint32,
    )

    # Ensure C-style contiguous arrays and correct dtypes as required by py::array flags
    origins = np.ascontiguousarray(origins, dtype=np.float32)
    directions = np.ascontiguousarray(directions, dtype=np.float32)
    vertices = np.ascontiguousarray(vertices, dtype=np.float32)
    indices = np.ascontiguousarray(indices, dtype=np.uint32)

    # Call the new signature: intersect(origins, directions, vertices, indices) -> np.ndarray[int]
    hits = optixbaker.intersect(origins, directions, vertices, indices)
    print("Hits:", hits)
