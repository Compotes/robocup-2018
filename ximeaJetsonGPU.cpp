// xiApiPlusOcvExample.cpp : program opens first camera, captures and displays 40 images

#include <stdio.h>
#include "xiApiPlusOcv.hpp"
#include <time.h>

using namespace std;
using namespace cv;
using namespace cv::cuda;

struct timespec t0, t1;
unsigned int counter = 0;

GpuMat outGpu, outGpu2, hsvGpu;
Mat dst;

byte *matToBytes(Mat image)
{
   int size = image.total() * image.elemSize();
   byte *bytes = new byte[size];  // you will have to delete[] that later
   std::memcpy(bytes,image.data,size * sizeof(byte));
   return bytes;
}

Mat *uMat;
//cv::SimpleBlobDetector detector;

void unwrapAndSave(Mat *mat, float a, float b, double s) {
  //printf("Unwrapping %.1f %.1f %.1f\n", a, b, s);
  OmniOperations *omni = new OmniOperations(
      mat->size().width / 2.0f,
      mat->size().height / 2.0f,
      mat->size().height / 2.0f,
      57.0f, 
      a, 
      b,
      s
      );

  // char fname[128];
  // sprintf(fname, "/tmp/test_omni_%.1f_%.1f_%.1f.png", a, b, s);
  uMat = omni->unwrap(*mat, false);
  //result->copyTo(uMat);

  //imwrite(fname, *result);

  delete omni;
}

void measureFps() {
    clock_gettime(CLOCK_REALTIME, &t1);
    uint64_t deltaTime = (t1.tv_sec - t0.tv_sec) * 1000000 + (t1.tv_nsec - t0.tv_nsec) / 1000 / 1000;
    counter++;
    if (deltaTime > 1000) {
      printf("FPS %10d\n", counter);
      counter = 0;
      t0 = t1;
    }
}

int main(int argc, char* argv[])
{
    namedWindow("input", WINDOW_AUTOSIZE);
    namedWindow("result", WINDOW_AUTOSIZE);
    namedWindow("gui", WINDOW_AUTOSIZE);

    moveWindow("input", 0, 0);
    moveWindow("result", 0, 1440/2);
    moveWindow("gui", 1300, 0);

	xiAPIplusCameraOcv cam;

	// Retrieving a handle to the camera device
	printf("Opening first camera...\n");
	cam.OpenFirst();
	
	// Set exposure
	//cam.SetExposureTime(10000); //10000 us = 10 ms
	cam.SetExposureTime(10000); //10000 us = 10 ms
	cam.SetDownsamplingType(XI_SKIPPING);
	cam.SetDownsampling(cam.GetDownsampling_Maximum());
	//cam.SetGain(20.0f);
	cam.SetImageDataFormat(XI_RAW8);
	
	printf("Starting acquisition...\n");
	cam.StartAcquisition();
	
	printf("First pixel value \n");
	XI_IMG_FORMAT format = cam.GetImageDataFormat();
	printf("XI_IMG_FORMAT=%d\n", format);

    printf("Get XI_PRM_COLOR_FILTER_ARRAY\n");
		XI_COLOR_FILTER_ARRAY colorFilterArray = cam.GetSensorColorFilterArray();
    printf("XI_PRM_COLOR_FILTER_ARRAY=%d\n", colorFilterArray);

    int val1 =  21; createTrackbar("Hmin",        "gui", &val1,  255, 0);
    int val2 =   0; createTrackbar("Smin",        "gui", &val2,  255, 0);
    int val3 =   0; createTrackbar("Vmin",        "gui", &val3,  255, 0);
    int val4 = 255; createTrackbar("Hmax",        "gui", &val4,  255, 0);
    int val5 = 255; createTrackbar("Smax",        "gui", &val5,  255, 0);
    int val6 = 255; createTrackbar("Vmax",        "gui", &val6,  255, 0);
    int val7 = 300; createTrackbar("elementSize", "gui", &val7, 1000, 0);
    int val8 =   0; createTrackbar("val8",        "gui", &val8,  255, 0);
		
    cv::Ptr<cv::xphoto::WhiteBalancer> wb = cv::xphoto::createSimpleWB();
    counter = 0;
    clock_gettime(CLOCK_REALTIME, &t0);

    GpuMat temp;
    GpuMat thres;
    GpuMat toInv;
		
    for (;;)
		{
			Mat mat = cam.GetNextImageOcvMat();
      
//      imshow("result", mat);
//      measureFps();
  //    cv::waitKey(1);

      const GpuMat inGpu(mat);
      cv::cuda::demosaicing(inGpu, outGpu, COLOR_BayerBG2BGR);
      Mat demosMat;
      outGpu.download(demosMat);
      
      // Mat testMat;
/*       imshow("result", demosMat);
       cv::waitKey(1);
       measureFps();
       continue;
*/

      //Mat wbMat;
      //wb->balanceWhite(demosMat, wbMat);

      //imshow("result", wbMat);
      //cv::waitKey(1);
      //continue;

      unwrapAndSave(&demosMat, 1.0f, 0.5f, 0.1);
      //imshow("unwrapkokotpica", demosMat);
      //cv::waitKey(1);

      // HSV threshold
      const GpuMat inUmat(*uMat);
      cv::cuda::cvtColor(inUmat, hsvGpu, COLOR_BGR2HSV);

      //Split HSV 3 channels
      GpuMat shsv[3];
      cv::cuda::split(hsvGpu, shsv);

      //Threshold HSV channels
      GpuMat thresc[3];
      cv::cuda::threshold(shsv[0], thresc[0], val1, val4, THRESH_BINARY);
      cv::cuda::threshold(shsv[1], thresc[1], val2, val5, THRESH_BINARY);
      cv::cuda::threshold(shsv[2], thresc[2], val3, val6, THRESH_BINARY);

      //Bitwise AND the channels
      cv::cuda::bitwise_and(thresc[0], thresc[1], temp);
      cv::cuda::bitwise_and(temp, thresc[2], toInv);
      cv::cuda::bitwise_not(toInv, thres);

      // Mat thrMat;
      // thres.download(thrMat);
      // Mat hsvMat;
      // hsvGpu.download(hsvMat);
      // Mat thrMat;
      // inRange(hsvMat, cv::Scalar(val1, val2, val3), cv::Scalar(val4, val5, val6), thrMat);



      //std::vector<KeyPoint> keypoints;
      //detector.detect(thrMat, keypoints);

      // Mat blobMat;
      // drawKeypoints(uMat, keypoints, blobMat, Scalar(0,0,255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

      //const GpuMat filterMat(thrMat);

      GpuMat filterMat = thres;
      GpuMat erodeMat, dilateMat;

      int elementShape = MORPH_RECT;
      float filterArea = val7 / 100.0f;
      Mat element = getStructuringElement(elementShape, Size(filterArea, filterArea), Point(filterArea / 2.0f, filterArea / 2.0f));
      Ptr<cuda::Filter> erodeFilter = cuda::createMorphologyFilter(MORPH_ERODE, filterMat.type(), element);
      erodeFilter->apply(filterMat, erodeMat);
      Ptr<cuda::Filter> dilateFilter = cuda::createMorphologyFilter(MORPH_DILATE, erodeMat.type(), element);
      dilateFilter->apply(erodeMat, dilateMat);

      Mat filteredMat;
      dilateMat.download(filteredMat);


      cv::SimpleBlobDetector::Params params;
      params.filterByColor = 1;
      params.blobColor = 255;
      params.minThreshold = 10;
      params.maxThreshold = 220;
      params.filterByArea = true;
      params.minArea = 0;
      params.filterByCircularity = true;
      params.minCircularity = 0.5f;
      params.maxCircularity = 1.0f;
      params.filterByConvexity = false;
      params.filterByInertia = false;
      cv::Ptr<cv::SimpleBlobDetector> detector = cv::SimpleBlobDetector::create(params);
      vector<KeyPoint> blobList;
      detector->detect(filteredMat, blobList);;

      //Mat testMat;
      //dilateMat.download(testMat);
      //imshow("result", testMat);
      //cv::waitKey(1);
      //measureFps();
      //continue;

      Mat blankMat = Mat::zeros(filteredMat.size(), filteredMat.type());
      Mat blobMat;
      drawKeypoints(blankMat, blobList, blobMat, Scalar(255, 255, 255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);


      Mat resultMat = blobMat;
      imshow("input", *uMat);
      imshow("result", resultMat);
      cv::waitKey(1);

      //break;


       //cv::imshow("jetson", *uMat);
       //cv::waitKey(1);


      //imwrite("/tmp/test_RGB24.png", dst);
      //imwrite("/tmp/test_RGB24_AWB.png", finalMat);
      //break;

      // int buffSize = (dst.total() * dst.channels());
      // byte *buff = matToBytes(dst);

      //FILE *fp=fopen("/tmp/test.img", "w+");
      //if (fp == NULL) {
      //  printf("Zle nedobre\n");
      //  break;
      //}
      //fwrite(buff, 1, buffSize, fp);
      //break;

      measureFps();
		}
		
		cam.StopAcquisition();
		cam.Close();
		printf("Done\n");	
}

