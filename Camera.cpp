#include "Camera.hpp"

using namespace std;
using namespace cv;
using namespace cv::cuda;

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
GpuMat ballErodeMat, goalErodeMat, greenErodeMat;
GpuMat ballDilateMat, goalDilateMat, greenDilateMat;

GpuMat ballGpuMat(WIDTH, HEIGHT, CV_8UC1);
GpuMat goalGpuMat(WIDTH, HEIGHT, CV_8UC1);
GpuMat greenGpuMat(WIDTH, HEIGHT, CV_8UC1);

Mat debugMerge(WIDTH, HEIGHT, CV_8UC3);

Mat goalMarker, ballMarker, goalMat, ballMat;

Mat mat;
Mat dst;
Mat thrMat;
Mat hsvMat;
Mat inputMat;
Mat resultMat;
Mat ballDilated;
Mat goalDilated;
Mat greenDilated;

// BGR
Scalar ballContourColor = Scalar(0, 0, 255);
Scalar goalContourColor = Scalar(0, 255, 255);
Scalar centerContourColor = Scalar(255, 33, 0);

const string BALL_FILE = "ball";
const string GREEN_FILE = "green";

vector<vector<Point>> ballContours, goalContours;
vector<Vec4i> ballHierarchy, goalHierarchy;

atomic<int> frame_rate;
atomic<int> live_stream;

atomic<bool> ball_visible;
atomic<int> ball_x;
atomic<int> ball_y;
atomic<int> ext_ball_zone;

atomic<bool> ball_close_kick;

atomic<int> goal_x;
atomic<int> goal_y;
atomic<int> goal_height;
atomic<int> goal_width;
atomic<bool> goal_visible;

atomic<bool> ext_i_see_goal_to_kick;

atomic<bool> ext_livestream;

atomic<bool> ext_attack_blue_goal;
bool attack_blue_goal;

InRangeParam ballInRangeParam, goalInRangeParam, greenInRangeParam;
int hMin, sMin, vMin;
int hMax, sMax, vMax;

int showImage, showFpsCount, imageWindowsOn;

//ofstream goal_values;
//myfile.open ("goal_values.txt");
//writing_counter = 0;

Mat element;
Ptr<cuda::Filter> erodeFilter;
Ptr<cuda::Filter> dilateFilter;
int elementShape = MORPH_ELLIPSE;
float filterArea;
int dilateMultiplier;

object goal, ball;

#define CALIBRATION true

#define MIN_GOAL_WIDTH 38
#define MIN_GOAL_HEIGHT 12

#define MIN_BALL_WIDTH 10
#define MIN_BALL_HEIGHT 10

#define GOAL_MAX_PIXEL 500
#define BALL_MAX_PIXEL 772

#define KICK_TOLERANCE 30

#define KICK_X_DIFFERENCE 70

#define BALL_Y_MIN_KICK 365
#define BALL_Y_MAX_KICK 500

struct timespec t0, t1;

xiAPIplusCameraOcv cam;

int ball_close(int x, int y) {
	//first zone
	float res = 0.0006484349*x*x-0.648445*x+499.613981 - FIRST_ZONE_TOLERANCE;
	if (res < y) return FIRST_ZONE_NUMBER;

	//second zone
	res = 0.0005064493*x*x-0.5180190126*x+351.566136 - SECOND_ZONE_TOLERANCE;
	if (res < y) return SECOND_ZONE_NUMBER;

	//third zone
	res = 0.0003762345*x*x-0.3793232269*x+268.615192 - THIRD_ZONE_TOLERANCE;
	if (res < y) return THIRD_ZONE_NUMBER;

	// fourth zone
	res = 0.0003288547*x*x-0.3320353119*x+227.712064 - FOURTH_ZONE_TOLERANCE;
	if (res < y) return FOURTH_ZONE_NUMBER;

	// fifth zone
	res = 0.0003011454*x*x-0.3028442648*x+201.358953 - FIFTH_ZONE_TOLERANCE;
	if (res < y) return FIFTH_ZONE_NUMBER;

	return BLYAT_ZONE_NUMBER;
}

byte *matToBytes(Mat image) {
    int size = image.total() * image.elemSize();
    byte *bytes = new byte[size];
    std::memcpy(bytes,image.data,size * sizeof(byte));
    return bytes;
}

Mat *uMat;

void overlayImage(const cv::Mat &background, const cv::Mat &foreground, cv::Mat &output, cv::Point2i location) {
  background.copyTo(output);


  // start at the row indicated by location, or at row 0 if location.y is negative.
  for(int y = std::max(location.y , 0); y < background.rows; ++y)
  {
    int fY = y - location.y; // because of the translation

    // we are done of we have processed all rows of the foreground image.
    if(fY >= foreground.rows)
      break;

    // start at the column indicated by location,

    // or at column 0 if location.x is negative.
    for(int x = std::max(location.x, 0); x < background.cols; ++x)
    {
      int fX = x - location.x; // because of the translation.

      // we are done with this row if the column is outside of the foreground image.
      if(fX >= foreground.cols)
        break;

      // determine the opacity of the foregrond pixel, using its fourth (alpha) channel.
      double opacity =
        ((double)foreground.data[fY * foreground.step + fX * foreground.channels() + 3])

        / 255.;


      // and now combine the background and foreground pixel, using the opacity,

      // but only if opacity > 0.
      for(int c = 0; opacity > 0 && c < output.channels(); ++c)
      {
        unsigned char foregroundPx =
          foreground.data[fY * foreground.step + fX * foreground.channels() + c];
        unsigned char backgroundPx =
          background.data[y * background.step + x * background.channels() + c];
        output.data[y*output.step + output.channels()*x + c] =
          backgroundPx * (1.-opacity) + foregroundPx * opacity;
      }
    }
  }
}

void measureFps() {
    clock_gettime(CLOCK_REALTIME, &t1);
    uint64_t deltaTime = (t1.tv_sec - t0.tv_sec) * 1000000 + (t1.tv_nsec - t0.tv_nsec) / 1000 / 1000;
    counter++;
    if (deltaTime > 1000) {
        fps = counter;
        frame_rate.store(fps);
        printf("\033[1;32;40mFPS:%d\033[0m\n", fps);
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
void debugImage(Mat *result) {
    if (debugCounter > showFpsCount+1) {
        char fpsStr[32];
        sprintf(fpsStr, "%10d", fps);
        rectangle(mat, Point2f(0, 0), Point2f(200, 40), Scalar(255, 255, 255, 255), CV_FILLED);
        putText(mat, fpsStr, Point2f(0, 30), FONT_HERSHEY_PLAIN, 2, Scalar(0,0,0,255));

        imshow("input", resultMat);

        if (result != NULL)
            imshow("result", *result);

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
    string color = attack_blue_goal ? "blue" : "yellow";
    camera_values.open("/root/robocup-2018/CAMERA_VALUES." + color, ios::out | ios::trunc);

    camera_values << goalInRangeParam.minH << endl;
    camera_values << goalInRangeParam.maxH << endl;
    camera_values << goalInRangeParam.minS << endl;
    camera_values << goalInRangeParam.maxS << endl;
    camera_values << goalInRangeParam.minV << endl;
    camera_values << goalInRangeParam.maxV << endl;

    camera_values.close();

    cout << "DATA SAVED" << endl;
}

void save_ball_values() {
	fstream camera_values;
	camera_values.open("/root/robocup-2018/CAMERA_VALUES." + BALL_FILE, ios::out | ios::trunc);

	camera_values << ballInRangeParam.minH << endl;
    camera_values << ballInRangeParam.maxH << endl;
    camera_values << ballInRangeParam.minS << endl;
    camera_values << ballInRangeParam.maxS << endl;
    camera_values << ballInRangeParam.minV << endl;
    camera_values << ballInRangeParam.maxV << endl;

    camera_values.close();

    cout << "BALL SAVED" << endl;
}

void save_green_values() {
	fstream camera_values;
	camera_values.open("/root/robocup-2018/CAMERA_VALUES." + GREEN_FILE, ios::out | ios::trunc);

	camera_values << greenInRangeParam.minH << endl;
    camera_values << greenInRangeParam.maxH << endl;
    camera_values << greenInRangeParam.minS << endl;
    camera_values << greenInRangeParam.maxS << endl;
    camera_values << greenInRangeParam.minV << endl;
    camera_values << greenInRangeParam.maxV << endl;

    camera_values.close();

    cout << "GREEN SAVED" << endl;
}

void load_ball_values() {
	string line;
    fstream camera_values;
	camera_values.open("/root/robocup-2018/CAMERA_VALUES." + BALL_FILE, ios::in);

    getline(camera_values, line); ballInRangeParam.minH = atoi(line.c_str());
    getline(camera_values, line); ballInRangeParam.maxH = atoi(line.c_str());
    getline(camera_values, line); ballInRangeParam.minS = atoi(line.c_str());
    getline(camera_values, line); ballInRangeParam.maxS = atoi(line.c_str());
    getline(camera_values, line); ballInRangeParam.minV = atoi(line.c_str());
    getline(camera_values, line); ballInRangeParam.maxV = atoi(line.c_str());

    cout << "BALL LOADED" << endl;

	camera_values.close();
}

void load_green_values() {
	string line;
    fstream camera_values;
	camera_values.open("/root/robocup-2018/CAMERA_VALUES." + GREEN_FILE, ios::in);

    getline(camera_values, line); greenInRangeParam.minH = atoi(line.c_str());
    getline(camera_values, line); greenInRangeParam.maxH = atoi(line.c_str());
    getline(camera_values, line); greenInRangeParam.minS = atoi(line.c_str());
    getline(camera_values, line); greenInRangeParam.maxS = atoi(line.c_str());
    getline(camera_values, line); greenInRangeParam.minV = atoi(line.c_str());
    getline(camera_values, line); greenInRangeParam.maxV = atoi(line.c_str());

    cout << "GREEN LOADED" << endl;

	camera_values.close();
}

bool goal_load_setter = false;

void load_values() {
    string line;
    fstream camera_values;
    string color = attack_blue_goal ? "blue" : "yellow";
    camera_values.open("/root/robocup-2018/CAMERA_VALUES." + color, ios::in);

    getline(camera_values, line); goalInRangeParam.minH = atoi(line.c_str());
    getline(camera_values, line); goalInRangeParam.maxH = atoi(line.c_str());
    getline(camera_values, line); goalInRangeParam.minS = atoi(line.c_str());
    getline(camera_values, line); goalInRangeParam.maxS = atoi(line.c_str());
    getline(camera_values, line); goalInRangeParam.minV = atoi(line.c_str());
    getline(camera_values, line); goalInRangeParam.maxV = atoi(line.c_str());


	if (goal_load_setter) {
		setTrackbarPos("goal minH", "gui2", goalInRangeParam.minH);
		setTrackbarPos("goal minS", "gui2", goalInRangeParam.minS);
		setTrackbarPos("goal minV", "gui2", goalInRangeParam.minV);
		setTrackbarPos("goal maxH", "gui2", goalInRangeParam.maxH);
		setTrackbarPos("goal maxS", "gui2", goalInRangeParam.maxS);
		setTrackbarPos("goal maxV", "gui2", goalInRangeParam.maxV);
	}

	goal_load_setter = true;

	cout << "DATA LOAD" << endl;

    camera_values.close();
}

void save_values_callback(int event, int x, int y, int flags, void* userdata) {
    if (event == EVENT_LBUTTONDOWN) {
        save_values();
        save_ball_values();
        save_green_values();
    }
}

void load_values_callback(int event, int x, int y, int flags, void* userdata) {
    if (event == EVENT_LBUTTONDOWN) {
        load_values();
        load_ball_values();
        load_green_values();
    }
}

void init_camera() {
    createImageWindows();

    namedWindow("gui", WINDOW_NORMAL);
    namedWindow("gui1", WINDOW_NORMAL);
    namedWindow("gui2", WINDOW_NORMAL);
    namedWindow("gui3", WINDOW_NORMAL);
    namedWindow("save", WINDOW_NORMAL);
    namedWindow("load", WINDOW_NORMAL);

    resizeWindow("gui", 200, 800);
    resizeWindow("gui1", 200, 800);
    resizeWindow("gui1", 200, 800);
    resizeWindow("gui2", 200, 800);
    resizeWindow("gui3", 200, 800);
    resizeWindow("save", 100, 100);
    resizeWindow("load", 100, 100);

    moveWindow("gui",   850, 0);
    moveWindow("gui1", 1050, 0);
    moveWindow("gui2", 1250, 0);
    moveWindow("gui3", 1450, 0);
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
    //cam.SetGain(12.0f);

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
    load_ball_values();
    load_green_values();

    createTrackbar("ball minH", "gui1", &ballInRangeParam.minH, 255, 0);
    createTrackbar("ball maxH", "gui1", &ballInRangeParam.maxH, 255, 0);
    createTrackbar("ball minS", "gui1", &ballInRangeParam.minS, 255, 0);
    createTrackbar("ball maxS", "gui1", &ballInRangeParam.maxS, 255, 0);
    createTrackbar("ball minV", "gui1", &ballInRangeParam.minV, 255, 0);
    createTrackbar("ball maxV", "gui1", &ballInRangeParam.maxV, 255, 0);
    ballInRangeParam.output = ballGpuMat;

    createTrackbar("goal minH", "gui2", &goalInRangeParam.minH, 255, 0);
    createTrackbar("goal maxH", "gui2", &goalInRangeParam.maxH, 255, 0);
    createTrackbar("goal minS", "gui2", &goalInRangeParam.minS, 255, 0);
    createTrackbar("goal maxS", "gui2", &goalInRangeParam.maxS, 255, 0);
    createTrackbar("goal minV", "gui2", &goalInRangeParam.minV, 255, 0);
    createTrackbar("goal maxV", "gui2", &goalInRangeParam.maxV, 255, 0);

    goalInRangeParam.output = goalGpuMat;

    createTrackbar("green minH", "gui3", &greenInRangeParam.minH, 255, 0);
    createTrackbar("green maxH", "gui3", &greenInRangeParam.maxH, 255, 0);
    createTrackbar("green minS", "gui3", &greenInRangeParam.minS, 255, 0);
    createTrackbar("green maxS", "gui3", &greenInRangeParam.maxS, 255, 0);
    createTrackbar("green minV", "gui3", &greenInRangeParam.minV, 255, 0);
    createTrackbar("green maxV", "gui3", &greenInRangeParam.maxV, 255, 0);

    greenInRangeParam.output = greenGpuMat;

    dilateMultiplier = 0; createTrackbar("dilate m", "gui", &dilateMultiplier,  900, update_filters);
    showImage = 0; createTrackbar("show image", "gui", &showImage,  1, 0);
    showFpsCount = 0; createTrackbar("show fps count", "gui", &showFpsCount,  100, 0);

	goalMat = imread("/root/robocup-2018/jerry.png", -1);
	ballMat = imread("/root/robocup-2018/supak.png", -1);

    thread camera_thread(update_camera);
    camera_thread.detach();
}

void update_filters(int, void*) {
    filterArea = (dilateMultiplier+100) / 100.0f;
    element = getStructuringElement(elementShape, Size(filterArea, filterArea), Point(filterArea / 2.0f, filterArea / 2.0f));
    erodeFilter = cuda::createMorphologyFilter(MORPH_ERODE, CV_8U, element);
    dilateFilter = cuda::createMorphologyFilter(MORPH_DILATE, CV_8U, element);
}

void write_to_livestream() {
	//string pipeline = "appsrc ! videoconvert ! vp8enc ! webmmux ! shout2send ip=127.0.0.1 port=1234 password=hackme mount=/s";
	string pipeline = "appsrc ! udpsink host=localhost port=9999";
	VideoWriter writer(pipeline, 0, (double)25, cv::Size(1032, 772), false);
	while(true) {
		writer << ballDilated;
	}
}

string pipeline = "appsrc ! videoconvert ! videoscale ! video/x-raw,width=500 ! clockoverlay shaded-background=true font-desc=\"Sans 38\" ! x264enc tune=\"zerolatency\" threads=1 ! tcpserversink host=0.0.0.0 port=4444";
VideoWriter writer(pipeline, 0, (double)25, cv::Size(1032, 772), false);

int calib_count = 666;

void distance_calibration(int count) {
	fstream calibration_data;
	int measuremets[772];
	int measuredCount[772];

	calibration_data.open("/root/robocup-2018/BALL_CALIB." + to_string(count), ios::out);
	cout << "calibration STARTS in" << endl;
	for (int i = 5; i > 0; i--) {
		cout << i << endl;
		usleep(1000*1000);
	}

	for(int i = 0; i < 15*100; i++) {
		if (ball.x != -1) {
			measuredCount[ball.y]++;
			measuremets[ball.y] += ball.width;
		}
		cout << i << endl;
		usleep(1000*10);
	}

	for (int i = 0; i < 772; i++) {
		if (measuremets[i] != 0)
			calibration_data << i << " " << measuremets[i] / measuredCount[i] << endl;
	}
	calib_count++;
	calibration_data.close();
}

void start_distance_calibration_thread() {
	thread calib(distance_calibration, calib_count);
	calib.detach();
}

void update_camera() {
    counter = 0;
    clock_gettime(CLOCK_REALTIME, &t0);

    GpuMat temp;
    GpuMat thres;
    GpuMat toInv;

    update_filters(0, NULL);

    //start_distance_calibration_thread();

    for (;;) {
		if (ext_attack_blue_goal.load() != attack_blue_goal) {
			attack_blue_goal = ext_attack_blue_goal.load();
			load_values();
		}

        mat = cam.GetNextImageOcvMat();

        inGpu.upload(mat);
        cv::cuda::demosaicing(inGpu, outGpu, FILTER);

        cv::cuda::cvtColor(outGpu, hsvGpu, COLOR_RGB2HSV);

        inRange_gpu(hsvGpu,
                ballInRangeParam.minH, ballInRangeParam.minS, ballInRangeParam.minV,
                ballInRangeParam.maxH, ballInRangeParam.maxS, ballInRangeParam.maxV,
                ballGpuMat);

        hsvGpuGoal = GpuMat(hsvGpu, Rect(0, 0, HEIGHT, GOAL_CROP_HEIGHT));

        inRange_gpu(hsvGpuGoal,
                goalInRangeParam.minH, goalInRangeParam.minS, goalInRangeParam.minV,
                goalInRangeParam.maxH, goalInRangeParam.maxS, goalInRangeParam.maxV,
                goalGpuMat);

        inRange_gpu(hsvGpu,
                greenInRangeParam.minH, greenInRangeParam.minS, greenInRangeParam.minV,
                greenInRangeParam.maxH, greenInRangeParam.maxS, greenInRangeParam.maxV,
                greenGpuMat);

        erodeFilter->apply(ballGpuMat, ballErodeMat);
        erodeFilter->apply(goalGpuMat, goalErodeMat);
        erodeFilter->apply(greenGpuMat, greenErodeMat);

        dilateFilter->apply(ballErodeMat, ballDilateMat);
        dilateFilter->apply(goalErodeMat, goalDilateMat);
        dilateFilter->apply(greenErodeMat, greenDilateMat);

        if (ext_livestream) {
			ballDilateMat.download(ballDilated);
			writer << ballDilated;
		}

        goal = goal_detect(goalDilateMat, MIN_GOAL_WIDTH, MIN_GOAL_HEIGHT, GOAL_MAX_PIXEL);
		ball //= goal_detect(ballDilateMat, MIN_BALL_WIDTH, MIN_BALL_HEIGHT, BALL_MAX_PIXEL);
		 = object_detect(ballDilateMat, MIN_BALL_WIDTH, MIN_BALL_HEIGHT, BALL_MAX_PIXEL);

        if (showImage)
            outGpu.download(resultMat); // DEBUG

		if (goal.x == -1) goal_visible.store(false);
		else goal_visible.store(true);

		if (ball.x == -1) ball_visible.store(false);
		else ball_visible.store(true);

		if(goal.x != -1) {
			if (ball.x > goal.x - goal.width / 2 + KICK_TOLERANCE && ball.x < goal.x + goal.width / 2 - KICK_TOLERANCE) {
				ext_i_see_goal_to_kick = true;
			} else {
				ext_i_see_goal_to_kick = false;
			}
		} else {
			ext_i_see_goal_to_kick = false;
		}

		ball_x.store(ball.x);
		ball_y.store(ball.y);
		ext_ball_zone.store(ball_close(ball.x, ball.y));

		goal_x.store(goal.x);
		goal_y.store(goal.y);
		goal_height.store(goal.height);
		goal_width.store(goal.width);

		//cout << ball.width << endl;

		if (showImage) {
			if (ext_i_see_goal_to_kick) {
				putText(resultMat, "KICK", Point2f(10, 25), FONT_HERSHEY_PLAIN, 2, Scalar(0,0,255,255));
			} else {
				putText(resultMat, "NO KICK", Point2f(10, 25), FONT_HERSHEY_PLAIN, 2, Scalar(0,255,0,255));
			}
			rectangle(resultMat, Point((HEIGHT/2)-KICK_X_DIFFERENCE, 365),
								 Point((HEIGHT/2)+KICK_X_DIFFERENCE, 500),
								 Scalar(0, 255, 255), 2, 8);
		}

		if ((ball.x > (HEIGHT/2)-KICK_X_DIFFERENCE && ball.x < (HEIGHT/2)+KICK_X_DIFFERENCE)
			&& (ball.y > BALL_Y_MIN_KICK && ball.y < BALL_Y_MAX_KICK)) {
			ball_close_kick = true;
		} else {
			ball_close_kick = false;
		}

		if (showImage) {
			if (goal.x != -1 && goal.height > 0) {
				rectangle(resultMat, Point(0, goal.y), Point(1032, goal.y), Scalar(0, 0, 255), 2, 8);
				rectangle(resultMat, Point(goal.x, 0), Point(goal.x, 772), Scalar(0, 0, 255), 2, 8);

				rectangle(resultMat, Point(goal.x - goal.width / 2 + KICK_TOLERANCE, 0),
						  Point(goal.x - goal.width / 2 + KICK_TOLERANCE, 772),
						  Scalar(0, 255, 0), 2, 8);

				rectangle(resultMat, Point(goal.x + goal.width / 2 - KICK_TOLERANCE, 0),
						  Point(goal.x + goal.width / 2 - KICK_TOLERANCE, 772),
						  Scalar(0, 255, 0), 2, 8);

				resize(goalMat,goalMarker,Size(goal.height*100/86,goal.height));
				overlayImage(resultMat, goalMarker, resultMat, cv::Point(goal.x - goal.height*100/86/2, goal.y - goal.height/2));
				//goalMarker.copyTo(resultMat(cv::Rect(goal.x - goalMarker.rows/2, goal.y - goalMarker.cols/2,
				//						goalMarker.cols, goalMarker.rows)));
			}

			if (ball.x != -1 && ball.width > 0 && ball.height > 0) {
				//rectangle(resultMat, Point(ball.x-ball.width/2, ball.y-ball.height/2), Point(ball.x+ball.width/2, ball.y+ball.height/2), Scalar(0, 255, 0), 2, 8);
				rectangle(resultMat, Point(0, ball.y), Point(1032, ball.y), Scalar(255, 0, 0), 2, 8);
				rectangle(resultMat, Point(ball.x, 0), Point(ball.x, 772), Scalar(255, 0, 0), 2, 8);
				resize(ballMat,ballMarker,Size(ball.width,ball.height));
				overlayImage(resultMat, ballMarker, resultMat, cv::Point(ball.x - ball.width/2, ball.y - ball.height/2));
				//ballMarker.copyTo(resultMat(cv::Rect(ball.x - ballMarker.rows/2, ball.y - ballMarker.cols/2,
				//						ballMarker.cols, ballMarker.rows)));
			}

			/*for(int j = 0; j < p_balls.size(); j++){
				rectangle(resultMat, Point(p_balls[j].x, p_balls[j].y), Point(p_balls[j].x+p_balls[j].width, p_balls[j].y+p_balls[j].height), Scalar(0, 255, 0), 2, 8);
			}*/

			  //cout << "Goal height: " << goal.height << " goal width: " << goal.width << "\n";
		}

		if(CALIBRATION && showImage) {
			outGpu.download(mat);
			goalDilateMat.download(goalDilated);
			ballDilateMat.download(ballDilated);
			greenDilateMat.download(greenDilated);

			auto channels = std::vector<cv::Mat>{Mat(WIDTH, HEIGHT, CV_8UC1, Scalar(0)), goalDilated, ballDilated, greenDilated};
			cv::merge(channels, debugMerge);

			createImageWindows();
			//debugImage(&greenDilated);
			debugImage(&debugMerge);
			continue;
		}

        // Utility
        if (showImage) {
            createImageWindows();
            outGpu.download(mat);
            debugImage(&resultMat);
        } else {
            measureFps();
            destroyImageWindows();
            if (cv::waitKey(1) == 27) {stop();}
        }
    }
    stop();
}
