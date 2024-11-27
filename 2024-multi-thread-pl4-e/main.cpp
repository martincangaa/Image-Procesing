/*
 * Main.cpp
 *
 *  Created on: Fall 2024
 */

#include <stdio.h>
#include <math.h>
#include <CImg.h>
#include <stdexcept>
#include <pthread.h>

using namespace cimg_library;

// Data type for image components
// Float
typedef float data_t;
// Constant of the number of threads
const unsigned short NUM_THREADS = 8;
uint pixelsPerThread;

const char* SOURCE_IMG      = "bailarina.bmp";
const char* SOURCE_IMG2     = "background_V.bmp";
const char* DESTINATION_IMG = "bailarina2.bmp";
const int FAILURE_GETTIME = -1;

// Filter argument data type
typedef struct {
	data_t *pIsrc; // Pointer to the R, G and B components of source image
	data_t *pIsrc2; // Pointer to the R, G and B components of 2nd source image

	data_t *pIdst; // Pointer to the destination image components (R,G,B)

	int pixelCount; // Size of the image in pixels
	int startPixel; // Pixel to start the algorithm
	int finishPixel; // Pixel to finish the algorithm
} filter_args_t;


filter_args_t filter_args;
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
void* filter (void* arg) {
	filter_args_t * filter_args = (filter_args_t *) arg;

    	for (int i = filter_args->startPixel; i < filter_args->finishPixel; i++) {

		*(filter_args->pIdst + i) = 255 - ((255 - *(filter_args->pIsrc + i)) * (255 - *(filter_args->pIsrc2 + i)) / 255);
	}

	return NULL;
}

int main() {
	// Open file and object initialization
	CImg<data_t> srcImage(SOURCE_IMG);
	CImg<data_t> srcImage2(SOURCE_IMG2);

	filter_args_t filter_args; // Filter arguments
	data_t *pDstImage; // Pointer to the new image pixels

	// Initialize the time variables
	struct timespec tStart, tEnd;
	double dElapsedTimeS;

	srcImage.display(); // Displays the source image
	int width = srcImage.width();
	int height = srcImage.height();
	filter_args.pixelCount = width * height;

	uint nComp = srcImage.spectrum();// source image number of components
	         // Common values for spectrum (number of image components):
				//  B&W images = 1
				//	Normal color images = 3 (RGB)
				//  Special color images = 4 (RGB and alpha/transparency channel)
	
	// Check if the images have the same size
	if (width != srcImage2.width() || height != srcImage2.height()) {
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
	filter_args.pIsrc = srcImage.data(); // pRcomp points to the R component array
	filter_args.pIsrc2 = srcImage2.data(); // pRcomp points to the R component array
	
	// Pointers to the RGB arrays of the destination image
	filter_args.pIdst = pDstImage;

	/***********************************************
	 * Measure initial time
	 */

	if(clock_gettime(CLOCK_REALTIME, &tStart) == FAILURE_GETTIME)
	{
		perror("clock_gettime");
		exit(EXIT_FAILURE);
	}
	//Set up threads

	/***********************************************
	 * Multi Threaded implementation:
	 *
	 * First create the struct for the thread
	 * Each one containing the pointers to the images and the start and finish pixels
	 * 
	 * Then creates the tasks for the thread passing as parameter
	 * its corresponding struct
	 */
	
	pthread_t threads[NUM_THREADS];
	filter_args_t thread_args[NUM_THREADS];
	uint pixelsPerThread = (filter_args.pixelCount / NUM_THREADS) * nComp;

	for(int t = 0; t < NUM_THREADS; t++)
	{
		thread_args[t] = filter_args;
		thread_args[t].startPixel = t * pixelsPerThread;
		
		/* The following condition ensures that all pixels from the image are passed 
		   through the algorithm since the number of pixels might not be a multiple of the number of threads
		   leaving some pixels without being processed
		*/	
		if(t == NUM_THREADS - 1) 
			thread_args[t].finishPixel = thread_args[t].startPixel + pixelsPerThread + (filter_args.pixelCount % NUM_THREADS);
		else
			thread_args[t].finishPixel = thread_args[t].startPixel + pixelsPerThread;

		int pthread_ret = pthread_create(&threads[t], NULL, filter, &thread_args[t]);
		if(pthread_ret){

			perror("Error creating thread");
			exit(EXIT_FAILURE);	
      
		}

		for(uint i = 0; i < NUM_THREADS; i++){
			pthread_join(threads[i], NULL);
		}
	}

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
	CImg<data_t> dstImage(pDstImage, width, height, 1, 3);

	// Store destination image in disk
	dstImage.save(DESTINATION_IMG); 

	// Display destination image
	dstImage.display();
	
	// Free memory
	free(pDstImage);

	return 0;
}
