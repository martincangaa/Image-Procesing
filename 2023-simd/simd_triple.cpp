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
 */
void filter (filter_args_t args) {
    
    __m128 cte_255 = _mm_set1_ps(255);

    int nElements = args.nPackets * args.items_per_packet;

    for (int i = 0; i < args.nPackets; i++) {
        if (args.pIsrc == nullptr || args.pIsrc2 == nullptr || args.pDstImage == nullptr) {
            fprintf(stderr, "Null pointer detected in filter arguments\n");
            return;
        }

        __m128 vIsrc = _mm_load_ps(args.pIsrc + (args.items_per_packet * i));
        __m128 vIsrc2 = _mm_load_ps(args.pIsrc2 + (args.items_per_packet * i));

        __m128 tmp1 = _mm_sub_ps(cte_255, vIsrc); // (255 - src1) 
        __m128 tmp2 = _mm_sub_ps(cte_255, vIsrc2); // (255 - src2)

        __m128 prdct = _mm_mul_ps(tmp1, tmp2); // (255 - src1) * (255 - src2)
        __m128 div = _mm_div_ps(prdct, cte_255); // ((255 - src1) * (255 - src2) / 255)

        __m128 result = _mm_sub_ps(cte_255, div); // 255 - ((255 - src1) * (255 - src2) / 255)

        _mm_store_ps(args.pDstImage + (args.items_per_packet * i), result);
    }

    // Calculation of the elements in va and vb in excess
    int dataInExcess = args.pixelCount % args.items_per_packet;
    if (dataInExcess > 0) {
        for (int i = nElements; i < args.pixelCount; i++) {
            if (args.pIsrc == nullptr || args.pIsrc2 == nullptr || args.pDstImage == nullptr) {
                fprintf(stderr, "Null pointer detected in filter arguments\n");
                return;
            }

            data_t src1 = args.pIsrc[i];
            data_t src2 = args.pIsrc2[i];

            data_t tmp1 = 255 - src1;
            data_t tmp2 = 255 - src2;

            data_t prdct = tmp1 * tmp2;
            data_t div = prdct / 255;

            args.pDstImage[i] = 255 - div;
        }
    }
}

int main() {
    // Load images
    CImg<data_t> img1(SOURCE_IMG);
    CImg<data_t> img2(SOURCE_IMG2);

    if (img1.width() != img2.width() || img1.height() != img2.height() || img1.spectrum() != img2.spectrum()) {
        fprintf(stderr, "Images must have the same dimensions\n");
        return -1;
    }

    int pixelCount = img1.width() * img1.height() * img1.spectrum();
    int items_per_packet = 4; // Number of floats in a SIMD register
    int nPackets = pixelCount / items_per_packet;

    // Allocate aligned memory
    data_t* pIsrc = (data_t*)_mm_malloc(pixelCount * sizeof(data_t), 16);
    data_t* pIsrc2 = (data_t*)_mm_malloc(pixelCount * sizeof(data_t), 16);
    data_t* pDstImage = (data_t*)_mm_malloc(pixelCount * sizeof(data_t), 16);

    if (!pIsrc || !pIsrc2 || !pDstImage) {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }

    // Copy image data to aligned memory
    memcpy(pIsrc, img1.data(), pixelCount * sizeof(data_t));
    memcpy(pIsrc2, img2.data(), pixelCount * sizeof(data_t));

    filter_args_t args = { pIsrc, pIsrc2, pDstImage, items_per_packet, nPackets, pixelCount };

    // Apply filter
    filter(args);

    // Save result
    CImg<data_t> resultImage(pDstImage, img1.width(), img1.height(), img1.depth(), img1.spectrum());
    resultImage.save(DESTINATION_IMG);

    // Free aligned memory
    _mm_free(pIsrc);
    _mm_free(pIsrc2);
    _mm_free(pDstImage);

    return 0;
}