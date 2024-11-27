/*
 * Main.cpp
 *
 *  Created on: Fall 2024
 */

#include <stdio.h>
#include <math.h>
#include <CImg.h>
#include <stdexcept>
#include <immintrin.h>

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
	data_t* pIsrc;
	data_t* pIsrc2;
	data_t* pDstImage;

	int items_per_packet;
	int nPackets;
	int pixelCount; // Size of the image in pixels
} filter_args_t;

/************************************************
 * Filter function
 * In order for the blend to work both images must have the same size.
 * 
 * Blend 2 images using the formula:
 * 	dst = 255 - ((255 - src1) * (255 - src2) / 255)
 * 
 * For each pixel, calculate the new value of the pixel (R, G, B) in the destination image
 * 
 * We only need one pointer per Image because if we go trough the pointer we'll reach
 * all the components of the image
 * 
 *       ┌─────┬─────┬─────┐ 
 *       │     │     │     │ 
 *       │  R  │  G  │  B  │ 
 *       │     │     │     │ 
 * 00x0h └─────┴─────┴─────┘ FFx0h 
 *
 */
void filter (filter_args_t args) {
	
	__m128 cte_255 = _mm_set1_ps(255);

	int nElements = args.nPackets * args.items_per_packet;

    for (int i = 0; i < args.nPackets; i++) {
		
		__m128 vIsrc = _mm_loadu_ps(args.pIsrc + (args.items_per_packet * i));
		__m128 vIsrc2 = _mm_loadu_ps(args.pIsrc2 + (args.items_per_packet * i));

		__m128 tmp1 = _mm_sub_ps(cte_255, vIsrc); // (255 - src1) 
		__m128 tmp2 = _mm_sub_ps(cte_255, vIsrc2); // (255 - src2)

		__m128 prdct = _mm_mul_ps(tmp1, tmp2); // (255 - src1) * (255 - src2)
		__m128 div = _mm_div_ps(prdct, cte_255); // ((255 - src1) * (255 - src2) / 255)

		__m128 result = _mm_sub_ps(cte_255, div); // 255 - ((255 - src1) * (255 - src2) / 255)

		_mm_store_ps(args.pDstImage + (args.items_per_packet * i), result);
	}

	// Calculation of the elements in va and vb in excess
	int dataInExcess = args.pixelCount % args.items_per_packet;

	for (int j = dataInExcess; j > 0; j++) {
		*(args.pDstImage + nElements - j) = 255 - ((255 - *(args.pIsrc + nElements - j)) * (255 - *(args.pIsrc2 + nElements - j)) / 255);
	}
}

int main() {
	// Open file and object initialization
	CImg<data_t> srcImage(SOURCE_IMG);
	CImg<data_t> srcImage2(SOURCE_IMG2);

	filter_args_t filter_args;
	float *pDstImage; // Pointer to the new image pixels

	// Initialize the time variables
	struct timespec tStart, tEnd;
	double dElapsedTimeS;


	srcImage.display(); // Displays the source image
	int width = srcImage.width();// Getting information from the source image
	int height = srcImage.height();	
	uint nComp = srcImage.spectrum();// source image number of components
	         // Common values for spectrum (number of image components):
				//  B&W images = 1
				//	Normal color images = 3 (RGB)
				//  Special color images = 4 (RGB and alpha/transparency channel)

	// Calculating image size in pixels
	filter_args.pixelCount = width * height * 3;

	filter_args.items_per_packet = sizeof(__m128) / sizeof(float);
	filter_args.nPackets = (filter_args.pixelCount * sizeof(float) / sizeof(__m128));

	
	// Check if the images have the same size
	if (width != srcImage.width() || height != srcImage.height()) {
		throw std::runtime_error("Images must have the same size"); // This adds robustness to the code
		exit(-1);
	}

	// Allocate memory space for destination image components
	pDstImage = (data_t*)_mm_malloc(sizeof(float) * filter_args.pixelCount, sizeof(__m128));
	if (pDstImage == NULL ) {
		perror("Allocating memory");
		exit(-2);
	}

	// Pointers to the componet arrays of the source images 1&2 to be used in the algorithm
	filter_args.pIsrc = srcImage.data();
	filter_args.pIsrc2 = srcImage2.data();
	
	// Pointers to the RGB arrays of the destination image
	filter_args.pDstImage = pDstImage;

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
	_mm_free(pDstImage);

	return 0;
}
