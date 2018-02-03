#include "Camera.hpp"

using namespace std;
using namespace cv;
using namespace cv::cuda;

#define INPUT_CROP_TOP 30
#define GOAL_CROP_HEIGHT 400

unsigned int counter = 0;
unsigned int fps = 0;

struct InRangeParam {
    int minH, maxH;
    int minS, maxS;
    int minV, maxV;
    cv::cuda::GpuMat output;
};

GpuMat hsvGpu;
GpuMat hsvGpuGoal;

GpuMat outGpu, outGpu2;
GpuMat shsv[3];
GpuMat thresch[3];
GpuMat threscl[3];
GpuMat thresc[3];
GpuMat inGpu;
GpuMat ballErodeMat, goalErodeMat;
GpuMat ballDilateMat, goalDilateMat;

GpuMat ballGpuMat(WIDTH, HEIGHT, CV_8UC1);;
GpuMat goalGpuMat(WIDTH, HEIGHT, CV_8UC1);;

Mat mat;
Mat dst;
Mat thrMat;
Mat hsvMat;
Mat inputMat;
Mat resultMat;
Mat ballDilated;
Mat goalDilated;

// BGR
Scalar ballContourColor = Scalar(0, 0, 255);
Scalar goalContourColor = Scalar(0, 255, 255);

vector<vector<Point> > ballContours, goalContours;
vector<Vec4i> ballHierarchy, goalHierarchy;

atomic<int> frame_rate;
atomic<int> live_stream;

atomic<bool> ball_visible;
atomic<int> ball_x;
atomic<int> ball_y;

atomic<int> goal_x;
atomic<int> goal_y;
atomic<bool> goal_visible;

InRangeParam ballInRangeParam, goalInRangeParam;
int hMin, sMin, vMin;
int hMax, sMax, vMax;

int showImage, showFpsCount, imageWindowsOn;

Mat element;
Ptr<cuda::Filter> erodeFilter;
Ptr<cuda::Filter> dilateFilter;
int elementShape = MORPH_RECT;
float filterArea;
int dilateMultiplier;


struct timespec t0, t1;

xiAPIplusCameraOcv cam;

byte *matToBytes(Mat image)
{
    int size = image.total() * image.elemSize();
    byte *bytes = new byte[size];
    std::memcpy(bytes,image.data,size * sizeof(byte));
    return bytes;
}

Mat *uMat;

void measureFps() {
    clock_gettime(CLOCK_REALTIME, &t1);
    uint64_t deltaTime = (t1.tv_sec - t0.tv_sec) * 1000000 + (t1.tv_nsec - t0.tv_nsec) / 1000 / 1000;
    counter++;
    if (deltaTime > 1000) {
        fps = counter;
        frame_rate.store(fps);
        printf("FPS:%10d ", fps);
        cout << "DEGREE: " << compass_degree.load() << endl;
        counter = 0;
        t0 = t1;
    }
}

void stop() {
    cam.StopAcquisition();
    cam.Close();
    printf("Done\n");
    exit(0);
}

int debugCounter = 0;
void debugImage(Mat *resultMat) {
    if (debugCounter > showFpsCount+1) {
        char fpsStr[32];
        sprintf(fpsStr, "%10d", fps);
        rectangle(mat, Point2f(0, 0), Point2f(200, 40), Scalar(255, 255, 255, 255), CV_FILLED);
        putText(mat, fpsStr, Point2f(0, 30), FONT_HERSHEY_PLAIN, 2, Scalar(0,0,0,255));

        imshow("input", mat);

        if (resultMat != NULL)
            imshow("result", *resultMat);

        debugCounter = 0;
    } else
        debugCounter++;

    if (cv::waitKey(1) == 27) {stop();}
    measureFps();
}

void createImageWindows() {
    if (imageWindowsOn == 0) {
        namedWindow("input", WINDOW_NORMAL);
        namedWindow("result", WINDOW_NORMAL);

        resizeWindow("input", 1000, 400);
        resizeWindow("result", 1000, 400);

        moveWindow("input", 0, 0);
        moveWindow("result", 0, 475);

        imageWindowsOn = 1;
    }
}

void destroyImageWindows() {
    if (imageWindowsOn == 1) {
        destroyWindow("input");
        destroyWindow("result");
    }
    imageWindowsOn = 0;
}

void save_values() {
    fstream camera_values;
    camera_values.open("CAMERA_VALUES.ahmed", ios::out | ios::trunc);

    camera_values << ballInRangeParam.minH << endl;
    camera_values << ballInRangeParam.maxH << endl;
    camera_values << ballInRangeParam.minS << endl;
    camera_values << ballInRangeParam.maxS << endl;
    camera_values << ballInRangeParam.minV << endl;
    camera_values << ballInRangeParam.maxV << endl;

    camera_values << goalInRangeParam.minH << endl;
    camera_values << goalInRangeParam.maxH << endl;
    camera_values << goalInRangeParam.minS << endl;
    camera_values << goalInRangeParam.maxS << endl;
    camera_values << goalInRangeParam.minV << endl;
    camera_values << goalInRangeParam.maxV << endl;

    camera_values.close();

    cout << "DATA SAVED" << endl;
}

void load_values() {
    string line;
    fstream camera_values;
    camera_values.open("CAMERA_VALUES.ahmed", ios::in);

    getline(camera_values, line); ballInRangeParam.minH = atoi(line.c_str());
    getline(camera_values, line); ballInRangeParam.maxH = atoi(line.c_str());
    getline(camera_values, line); ballInRangeParam.minS = atoi(line.c_str());
    getline(camera_values, line); ballInRangeParam.maxS = atoi(line.c_str());
    getline(camera_values, line); ballInRangeParam.minV = atoi(line.c_str());
    getline(camera_values, line); ballInRangeParam.maxV = atoi(line.c_str());

    getline(camera_values, line); goalInRangeParam.minH = atoi(line.c_str());
    getline(camera_values, line); goalInRangeParam.maxH = atoi(line.c_str());
    getline(camera_values, line); goalInRangeParam.minS = atoi(line.c_str());
    getline(camera_values, line); goalInRangeParam.maxS = atoi(line.c_str());
    getline(camera_values, line); goalInRangeParam.minV = atoi(line.c_str());
    getline(camera_values, line); goalInRangeParam.maxV = atoi(line.c_str());

    camera_values.close();
}

void save_values_callback(int event, int x, int y, int flags, void* userdata) {
    if (event == EVENT_LBUTTONDOWN) {
        save_values();
    }
}

void load_values_callback(int event, int x, int y, int flags, void* userdata) {
    if (event == EVENT_LBUTTONDOWN) {
        load_values();
    }
}

void init_camera() {
    createImageWindows();

    namedWindow("gui", WINDOW_NORMAL);

    namedWindow("save", WINDOW_NORMAL);
    namedWindow("load", WINDOW_NORMAL);

    resizeWindow("gui", 200, 800);
    resizeWindow("save", 100, 100);
    resizeWindow("load", 100, 100);

    moveWindow("gui", 1450, 0);
    moveWindow("save", 1100, 500);
    moveWindow("load", 1100, 700);

    setMouseCallback("save", save_values_callback);
    setMouseCallback("load", load_values_callback);

    printf("Opening first camera...\n");
    cam.OpenFirst();

    cam.SetExposureTime(EXPOSURE_TIME);
    cam.SetDownsamplingType(XI_SKIPPING);
    cam.SetDownsampling(cam.GetDownsampling_Maximum());
    cam.SetImageDataFormat(XI_RAW8);
    cam.SetNextImageTimeout_ms(CAMERA_TIMEOUT);

    printf("Starting acquisition...\n");
    cam.StartAcquisition();

    printf("First pixel value \n");
    XI_IMG_FORMAT format = cam.GetImageDataFormat();
    printf("XI_IMG_FORMAT=%d\n", format);

    printf("Get XI_PRM_COLOR_FILTER_ARRAY\n");
    XI_COLOR_FILTER_ARRAY colorFilterArray = cam.GetSensorColorFilterArray();
    printf("XI_PRM_COLOR_FILTER_ARRAY=%d\n", colorFilterArray);

    //hMin =  95; createTrackbar("Hmin",        "gui", &hMin,  255, 0);
    //sMin =   0; createTrackbar("Smin",        "gui", &sMin,  255, 0);
    //vMin =   0; createTrackbar("Vmin",        "gui", &vMin,  255, 0);
    //hMax = 110; createTrackbar("Hmax",        "gui", &hMax,  255, 0);
    //sMax = 255; createTrackbar("Smax",        "gui", &sMax,  255, 0);
    //vMax = 255; createTrackbar("Vmax",        "gui", &vMax,  255, 0);

    load_values();

    createTrackbar("ball minH", "gui", &ballInRangeParam.minH, 255, 0);
    createTrackbar("ball maxH", "gui", &ballInRangeParam.maxH, 255, 0);
    createTrackbar("ball minS", "gui", &ballInRangeParam.minS, 255, 0);
    createTrackbar("ball maxS", "gui", &ballInRangeParam.maxS, 255, 0);
    createTrackbar("ball minV", "gui", &ballInRangeParam.minV, 255, 0);
    createTrackbar("ball maxV", "gui", &ballInRangeParam.maxV, 255, 0);
    ballInRangeParam.output = ballGpuMat;

    createTrackbar("goal minH", "gui", &goalInRangeParam.minH, 255, 0);
    createTrackbar("goal maxH", "gui", &goalInRangeParam.maxH, 255, 0);
    createTrackbar("goal minS", "gui", &goalInRangeParam.minS, 255, 0);
    createTrackbar("goal maxS", "gui", &goalInRangeParam.maxS, 255, 0);
    createTrackbar("goal minV", "gui", &goalInRangeParam.minV, 255, 0);
    createTrackbar("goal maxV", "gui", &goalInRangeParam.maxV, 255, 0);

    goalInRangeParam.output = goalGpuMat;

    dilateMultiplier = 0; createTrackbar("dilate m", "gui", &dilateMultiplier,  900, update_filters);
    showImage = 1; createTrackbar("show image", "gui", &showImage,  1, 0);
    showFpsCount = 0; createTrackbar("show fps count", "gui", &showFpsCount,  100, 0);

    thread camera_thread(update_camera);
    camera_thread.detach();
}

void update_filters(int, void*) {
    filterArea = (dilateMultiplier+100) / 100.0f;
    element = getStructuringElement(elementShape, Size(filterArea, filterArea), Point(filterArea / 2.0f, filterArea / 2.0f));
    erodeFilter = cuda::createMorphologyFilter(MORPH_ERODE, CV_8U, element);
    dilateFilter = cuda::createMorphologyFilter(MORPH_DILATE, CV_8U, element);
}

void update_camera() {
    counter = 0;
    clock_gettime(CLOCK_REALTIME, &t0);

    GpuMat temp;
    GpuMat thres;
    GpuMat toInv;

    update_filters(0, NULL);

    int live_stream_frame_count = 0;

    for (;;) {
        mat = cam.GetNextImageOcvMat();

        inGpu.upload(mat);
        cv::cuda::demosaicing(inGpu, outGpu, FILTER);

        cv::cuda::cvtColor(outGpu, hsvGpu, COLOR_RGB2HSV);
        //inRange_gpu(hsvGpu, hMin, sMin, vMin, hMax, sMax, vMax, thresholded);
        inRange_gpu(hsvGpu,
                ballInRangeParam.minH, ballInRangeParam.minS, ballInRangeParam.minV, 
                ballInRangeParam.maxH, ballInRangeParam.maxS, ballInRangeParam.maxV,
                ballGpuMat);

        hsvGpuGoal = GpuMat(hsvGpu, Rect(0, 0, HEIGHT, GOAL_CROP_HEIGHT));

        inRange_gpu(hsvGpuGoal,
                goalInRangeParam.minH, goalInRangeParam.minS, goalInRangeParam.minV,
                goalInRangeParam.maxH, goalInRangeParam.maxS, goalInRangeParam.maxV,
                goalGpuMat);
        //inRange_gpu_multi(hsvGpu, ballInRangeParam, goalInRangeParam);

        erodeFilter->apply(ballGpuMat, ballErodeMat);
        erodeFilter->apply(goalGpuMat, goalErodeMat);

        dilateFilter->apply(ballErodeMat, ballDilateMat);
        dilateFilter->apply(goalErodeMat, goalDilateMat);

        ballDilateMat.download(ballDilated);
        goalDilateMat.download(goalDilated);

        findContours(ballDilated, ballContours, ballHierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
        findContours(goalDilated, goalContours, goalHierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

        if (showImage)
            outGpu.download(resultMat); // DEBUG

        //rectangle(resultMat, Point(0, 0), Point(WIDTH, INPUT_CROP_TOP), Scalar(255, 0, 0), 2, 8);

        // Ball processing
        bool ballVisible = false;
        for(unsigned int i = 0; i < ballContours.size(); i++ )
        {
            double area = contourArea(ballContours[i]);
            if (area < 200.0) continue;
            ballVisible = true;
            if (showImage)
                drawContours(resultMat, ballContours, i, ballContourColor, 2, 8, ballHierarchy, 0, Point() );
            Moments m = moments(ballContours[i]);
            int cx = m.m10 / m.m00;
            int cy = m.m01 / m.m00;

            ball_x.store(cx);
            ball_y.store(cy);
        }
        ball_visible.store(ballVisible);

        // Goal processing
        bool goalVisible = false;
        for(unsigned int i = 0; i < goalContours.size(); i++ )
        {
            double area = contourArea(goalContours[i]);
            if (area < 2000.0) continue;
            Moments m = moments(goalContours[i]);
            int cx = m.m10 / m.m00;
            int cy = m.m01 / m.m00;
            if (cy > GOAL_CROP_HEIGHT) continue;

            goalVisible = true;

            if (showImage)
                drawContours(resultMat, goalContours, i, goalContourColor, 2, 8, goalHierarchy, 0, Point() );

            goal_x.store(cx);
            goal_y.store(cy);
        }

        goal_visible.store(goalVisible);

        // Utility
        if (live_stream.load() == 1 && live_stream_frame_count > 50) {
            imwrite("/mnt/ramdisk/obr.jpg", resultMat);
            live_stream_frame_count = 0;
        }

        if (showImage) {
            createImageWindows();
            outGpu.download(mat);
            debugImage(&resultMat);
        } else {
            measureFps();
            destroyImageWindows();
            if (cv::waitKey(1) == 27) {stop();}
        }

        live_stream_frame_count++;
    }
    stop();
}
