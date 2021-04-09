/*
 *
 *  Example by Berak
 *  Source: https://answers.opencv.org/question/77638/cvqueryframe-always-return-null/
 *
 */
#include "opencv2/opencv.hpp"

#define CAPTURE_APP

using namespace cv;


int capture_write(int dev)
{
    VideoCapture cap(dev); // open the default camera
    if(!cap.isOpened())  // check if we succeeded
    {
        printf("Device is not opened\n");
        return -1;
    }

    Mat frame;
    printf("Start Capture\n");
    cap >> frame; // get a new frame from camera

    printf("Start write image to file\n");
    bool retval = false; 
    retval = imwrite("./cap.png", frame);// save image to file
    if (retval == false)
    {
        printf("Save image failed\n");
        return -1;
    }
    printf("Image saved\n");

    // the camera will be deinitialized automatically in VideoCapture destructor
    return 0;
}

#ifdef CAPTURE_APP
int main( int argc, char** argv )
{
    int dev=0;

    if(argc > 1)
    {
        // use /dev/video<# >
        sscanf(argv[1], "%d", &dev);
        printf("using %s\n", argv[1]);
    }
    else if(argc == 1)
        // use default /dev/video0
        printf("using default\n");

    else
    {
        // specific usage
        printf("usage: capture [dev]\n");
        exit(-1);
    }
    printf("Start Capture and write\n");
    int retval = -1;
    retval = capture_write(dev);
    if (retval < 0)
    {
        printf("error in capture_write function\n");
        return -1;
    }
    // the camera will be deinitialized automatically in VideoCapture destructor
    return 0;
}
#endif