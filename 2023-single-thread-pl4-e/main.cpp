/*
 * Main.cpp
 *
 *  Created on: Fall 2019
 */

#include <stdio.h>
#include <math.h>
#include <CImg.h>
#include <stdexcept>

using namespace cimg_library;

// Data type for image components
// Float
typedef float data_t;

const char* SOURCE_IMG      = "bailarina.bmp";
const char* SOURCE_IMG2     = "background_V.bmp";
const char* DESTINATION_IMG = "bailarina2.bmp";
const int FAILURE_GETTIME = -1;

// Filter argument data type
typedef struct {
	data_t *pRsrc; // Pointers to the R, G and B components
	data_t *pGsrc;
	data_t *pBsrc;
	data_t *pRdst;
	data_t *pGdst;
	data_t *pBdst;

	data_t *pRsrc2; // Pointers to the R, G and B components
	data_t *pGsrc2;
	data_t *pBsrc2;

	uint pixelCount; // Size of the image in pixels
} filter_args_t;

/************************************************
 * Filter function
 * In order for the blend to work both images must have the same size.
 * 
 * Blend 2 images using the formula:
 * 	dst = 255 - ((255 - src1) * (255 - src2) / 255)
 * 
 * For each pixel, calculate the new value of the pixel (R, G, B) in the destination image
 */
void filter (filter_args_t args) {
	
    for (uint i = 0; i < args.pixelCount; i++) {
		
		*(args.pRdst + i) = 255 - ((255 - *(args.pRsrc + i)) * (255 - *(args.pRsrc2 + i)) / 255);
		*(args.pGdst + i) = 255 - ((255 - *(args.pGsrc + i)) * (255 - *(args.pGsrc2 + i)) / 255);
		*(args.pBdst + i) = 255 - ((255 - *(args.pBsrc + i)) * (255 - *(args.pBsrc2 + i)) / 255);
	}

}

int main() {
	// Open file and object initialization
	CImg<data_t> srcImage(SOURCE_IMG);
	CImg<data_t> srcImage2(SOURCE_IMG2);

	filter_args_t filter_args;
	data_t *pDstImage; // Pointer to the new image pixels

	// Initialize the time variables
	struct timespec tStart, tEnd;
	double dElapsedTimeS;


	srcImage.display(); // Displays the source image
	uint width = srcImage.width();// Getting information from the source image
	uint height = srcImage.height();	
	uint nComp = srcImage.spectrum();// source image number of components
	         // Common values for spectrum (number of image components):
				//  B&W images = 1
				//	Normal color images = 3 (RGB)
				//  Special color images = 4 (RGB and alpha/transparency channel)

	// Calculating image size in pixels
	filter_args.pixelCount = width * height;
	
	// Check if the images have the same size
	if (srcImage2.width() != srcImage.width() || srcImage2.height() != srcImage.height()) {
		throw std::runtime_error("Images must have the same size"); // This adds robustness to the code
		exit(-1);
	}

	// Allocate memory space for destination image components
	pDstImage = (data_t *) malloc (filter_args.pixelCount * nComp * sizeof(data_t));
	if (pDstImage == NULL) {
		perror("Allocating destination image");
		exit(-2);
	}

	// Pointers to the componet arrays of the source images 1&2 to be used in the algorithm
	filter_args.pRsrc = srcImage.data(); // pRcomp points to the R component array
	filter_args.pGsrc = filter_args.pRsrc + filter_args.pixelCount; // pGcomp points to the G component array
	filter_args.pBsrc = filter_args.pGsrc + filter_args.pixelCount; // pBcomp points to B component array
	filter_args.pRsrc2 = srcImage2.data(); // pRcomp points to the R component array
	filter_args.pGsrc2 = filter_args.pRsrc2 + filter_args.pixelCount; // pGcomp points to the G component array
	filter_args.pBsrc2 = filter_args.pGsrc2 + filter_args.pixelCount; // pBcomp points to B component array
	
	// Pointers to the RGB arrays of the destination image
	filter_args.pRdst = pDstImage;
	filter_args.pGdst = filter_args.pRdst + filter_args.pixelCount;
	filter_args.pBdst = filter_args.pGdst + filter_args.pixelCount;


	/***********************************************
	 * Measure initial time
	 */

	if(clock_gettime(CLOCK_REALTIME, &tStart) == FAILURE_GETTIME)
	{
		perror("clock_gettime");
		exit(EXIT_FAILURE);
	}

	/************************************************
	 * Algorithm.
	 */
	filter(filter_args);


	/***********************************************
	 *   - Measure the end time
	 *   - Calculate the elapsed time
	 */
	
	if(clock_gettime(CLOCK_REALTIME, &tEnd) == FAILURE_GETTIME)
	{
		perror("clock_gettime");
		exit(EXIT_FAILURE);
	}
 	
	printf("Finished\n");

	dElapsedTimeS = (tEnd.tv_sec - tStart.tv_sec);
    dElapsedTimeS += (tEnd.tv_nsec - tStart.tv_nsec) / 1e+9;
	printf("Elapsed time    : %f s.\n", dElapsedTimeS);

	// Create a new image object with the calculated pixels
	// In case of normal color images use nComp=3,
	// In case of B/W images use nComp=1.
	CImg<data_t> dstImage(pDstImage, width, height, 1, nComp);

	// Store destination image in disk
	dstImage.save(DESTINATION_IMG); 

	// Display destination image
	dstImage.display();
	
	// Free memory
	free(pDstImage);

	return 0;
}