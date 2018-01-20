CXX=g++
SOURCES=ximeaJetsonGPU.cpp
SOURCES+=xiApiPlusOcv.cpp
SOURCES+=
OBJECTS=$(SOURCES:.cpp=.o)
OBJECTS+=
PROGRAM=ximeaJetsonGPU

CUDAPATH = /usr/local/cuda-8.0

#LFLAGS = -L$(CUDAPATH)/lib64 -L/root/opencv-3.3.0/build/lib -lcuda -lcudart -lopencv_cudabgsegm -lopencv_cudaobjdetect -lopencv_cudastereo -lopencv_dnn -lopencv_ml -lopencv_shape -lopencv_stitching -lopencv_cudafeatures2d -lopencv_superres -lopencv_cudacodec -lopencv_videostab -lopencv_cudaoptflow -lopencv_cudalegacy -lopencv_calib3d -lopencv_features2d -lopencv_highgui -lopencv_videoio -lopencv_photo -lopencv_imgcodecs -lopencv_cudawarping -lopencv_cudaimgproc -lopencv_cudafilters -lopencv_video -lopencv_objdetect -lopencv_imgproc -lopencv_flann -lopencv_cudaarithm -lopencv_core -lopencv_cudev
LFLAGS = 

all: $(PROGRAM)

$(PROGRAM): xiApiPlusOcv.o ximeaJetsonGPU.o
	g++ $(OBJECTS) -o $@ -lm3api -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_imgcodecs -lopencv_cudaimgproc -lopencv_cudafilters -lcudart -lopencv_features2d -lopencv_cudaarithm -L/root/opencv-3.3.0/build/lib -L$(CUDAPATH)/lib64

xiApiPlusOcv.o: xiApiPlusOcv.cpp
	g++ -c $< -I . -I /root/opencv-3.3.0/build/include -I /root/opencv_contrib-3.3.0/modules/xphoto/include -I /usr/local/cuda/include -g3 -Wall -c -fmessage-length=0

ximeaJetsonGPU.o: ximeaJetsonGPU.cpp
	g++ -c $< -I .  -I /root/opencv-3.3.0/build/include -I /root/opencv_contrib-3.3.0/modules/xphoto/include -I /usr/local/cuda/include -g3 -Wall -c -fmessage-length=0

runX11: $(PROGRAM)
	cat ~nvidia/.Xauthority > ~/.Xauthority
	LD_LIBRARY_PATH=/root/opencv-3.3.0/build/lib DISPLAY=localhost:10.0 ./ximeaJetsonGPU

run: $(PROGRAM)
	cat ~nvidia/.Xauthority > ~/.Xauthority
	LD_LIBRARY_PATH=/root/opencv-3.3.0/build/lib ./ximeaJetsonGPU

runGui: $(PROGRAM)
	rm -f /tmp/*.png
	cat ~nvidia/.Xauthority > ~/.Xauthority
	LD_LIBRARY_PATH=/root/opencv-3.3.0/build/lib DISPLAY=:0 ./ximeaJetsonGPU

clean:
	rm -f *.o *~ $(PROGRAM) $(OBJECTS)
