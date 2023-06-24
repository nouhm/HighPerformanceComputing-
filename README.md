# HighPerformanceComputing-
Parallel High Pass Filter Implementation with OpenMP and MPI to make an image appear sharper.

Note, if there is no change in intensity, nothing happens. But if one pixel is brighter than its neighbors, it gets boosted. High pass filters amplifies noise. It allows high frequency components of the image to pass through and block low frequencies. The image will The idea is the same as the Low Pass Filter above but with using another kernel which is [[0, -1, 0], [-1, 4, -1], [0, -1, 0]].


