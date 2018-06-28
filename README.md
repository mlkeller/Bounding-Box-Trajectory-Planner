Bounding Box Trajectory Planner

Usage: DisplayImage.out <Image_Path> <lowThreshold> <highThreshold> <blurKernalSize> <imageWidth> <imageHeight>

Low Threshold and High Threshold are for generating contours.
BlurKernalSize - The kernal size for the blur pass. (Use 7)
Image Width - Number of cell columns given with the info map.
Image Height - Number of cell rows given with the info map.

Key Commands:
	q: Increase Low Threshold
	a: Decrease Low Threshold
	w: Increase High Threshold
	s: Decrease High Threshold
	+: Increase Blur Kernal Size
	-: Decrease Blur Kernal Size
	Escape: End Program

The Bounding Box Trajectory Planner (BBTP) takes in an Info Map in order to produce a two-dimensional bounding box based on areas of high interest. Using several of the methods provided by OpenCV, it generates and outputs a path whose points are in pixel space.

