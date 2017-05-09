#include "ImgProc/ImgProc.h"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
//#include "ImgProc/cuda/ImgProcCuda.h"
#include "ImgProc/LaneDetector.h"

#include "OpenNI2_Helper.h"

#define RAW_WIDTH 320
#define RAW_HEIGHT 240
#define FPS 30
#define SAMPLE_READ_WAIT_TIMEOUT 2000 //2000 ms

openni::Device	* p_device;

int main()
{
	openni::VideoStream	depthStream, colorStream;
	openni::VideoFrameRef		m_depthFrame, m_colorFrame;
	
	openni::VideoStream* streams[] = { &depthStream, &colorStream };
	setupAstra(p_device, depthStream, colorStream, RAW_WIDTH, RAW_HEIGHT, FPS);

	ImgProc3D::LaneDetector lane_detector(RAW_WIDTH,RAW_HEIGHT);
	lane_detector.setCamera(ImgProc3D::IntrMode_320x240_RAW);
	bool programShouldClose = false;

	while (!programShouldClose)
	{
		int changedStreamDummy;
		char rc = openni::OpenNI::waitForAnyStream(streams, 1, &changedStreamDummy, SAMPLE_READ_WAIT_TIMEOUT);
		if (rc != openni::STATUS_OK)	
		{
			printf("Wait failed! (timeout is %d ms)\n%s\n", SAMPLE_READ_WAIT_TIMEOUT, openni::OpenNI::getExtendedError());
			continue;
		}
		if (rc != openni::STATUS_OK){
			printf("Read failed!\n%s\n", openni::OpenNI::getExtendedError());
			continue;
		}

		int64 st = cv::getTickCount();
		colorStream.readFrame(&m_colorFrame);
		depthStream.readFrame(&m_depthFrame);

		cv::Mat depthMat = cv::Mat(RAW_HEIGHT, RAW_WIDTH, CV_16UC1, (openni::DepthPixel *)m_depthFrame.getData());
		//depthMat *= 5;

		cv::Mat bgrMat;
		cv::Mat colorRaw = cv::Mat(RAW_HEIGHT, RAW_WIDTH, CV_8UC3, (uint8_t *)m_colorFrame.getData());
		//cv::cvtColor(colorRaw, bgrMat, CV_RGB2BGR);

		lane_detector.processFrame(colorRaw, depthMat);

		cv::Mat gray_img;
		cv::cvtColor(lane_detector.laneMap2D, gray_img, cv::COLOR_BGR2GRAY);
		std::vector<cv::Point2f> centers = lane_detector.findLaneCenter(gray_img, 65);

		printf("Detect time = %f \n", (cv::getTickCount() - st) / cv::getTickFrequency());

		for (int i = 0; i < centers.size(); i++)
		{
			cv::circle(lane_detector.laneMap2D, cv::Point2f(centers[i].x, 512 - centers[i].y), 3, cv::Scalar(100, 250, 0), 2);
		}

		cv::Point2f centerP;
		float angel;

		if (lane_detector.detectLaneCenter(gray_img, centerP)) {
			std::cout << centerP << std::endl;
			cv::circle(lane_detector.laneMap2D, cv::Point2f(centerP.x, 512 - centerP.y), 5, cv::Scalar(0, 0, 255), 2);
		}

		cv::imshow("LANE", lane_detector.laneMap2D);
		//cv::waitKey(5);

		cv::imshow("cl", colorRaw);
		cv::imshow("d", depthMat);
		char key = cv::waitKey(5);
		if (key == 'q')
		{
			programShouldClose = true;
		}
	}

	depthStream.stop();
	depthStream.destroy();
	colorStream.stop();
	colorStream.destroy();

	openni::OpenNI::shutdown();
	return 0;
}