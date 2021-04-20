/**************************************************************************************************
* @file        server.cpp
* @version     0.1.1
* @type:       OpenCV Video streaming socket application
* @brief       OpenCV application that provides video stream to client requests.
 		  - Main Thread: Generate video stream and write image data to a global structure
 		  - Innitializes a socket connection to support client requests for video stream
 		  - Each client request is handled by a separate thread
* @author      Julian Abbott-Whitley (julian.abbott-whitley@Colorado.edu)
* @license:    GNU GPLv3   (attached below)
*
* @references: The following sources were referenced during development
*              https://gist.github.com/Tryptich/2a15909e384b582c51b5
*
**************************************************************************************************/

#include "opencv2/opencv.hpp"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include "facedetect.h"
#include "queue.h"



using namespace cv;

// Globals 
char userInput;
char endProgram;


typedef struct
{
	int dev;
	int imgSize;
	int time_sleep;
	int face_detected;
	bool face_detect_enable;
	Mat img;
	Mat imgGray;
	VideoCapture *cap;
	CascadeClassifier cascade;
	CascadeClassifier nestedCascade;

} ImgCaptureStruct;


struct VideoStreamStruct
{
	int remoteSocket;
	pthread_t thread_id;
	ImgCaptureStruct *imgStruct;
	SLIST_ENTRY(VideoStreamStruct) entries;
};

typedef VideoStreamStruct VideoStream;

void *get_user_input(void *ptr);
void *display(void *);
void *capture_video(void *);
void setup_img(ImgCaptureStruct *);


int main(int argc, char** argv)
{


    //--------------------------------------------------------
    // Setup network configuration settings: socket, bind, listen
    //--------------------------------------------------------
    int localSocket;
    int port;

    struct  sockaddr_in localAddr,
                        remoteAddr;

    pthread_t user_input_tid;
    pthread_t camera_processor_tid;
    int addrLen = sizeof(struct sockaddr_in);

    if ( (argc > 1) && (strcmp(argv[1],"-h") == 0) ) 
    {
          std::cerr << "usage: ./cv_video_srv [port] [capture device]\n" <<
                       "port           : socket port (4097 default)\n" <<
                       "capture device : (0 default)\n" << std::endl;

          exit(1);
    }


    if (argc >= 2) port = atoi(argv[1]);

    localSocket = socket(AF_INET , SOCK_STREAM , 0);
    if (localSocket == -1){
         perror("socket() call failed!!");
    }

    int enable = 1;
    if (setsockopt(localSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(4099);

    if( bind(localSocket,(struct sockaddr *)&localAddr , sizeof(localAddr)) < 0) {
         perror("Can't bind() socket");
         exit(1);
    }


    //-------------------------------------------------------
    // Innitialize ImgCaptureStruct members 
    //-------------------------------------------------------
    ImgCaptureStruct imgStruct;
    imgStruct.dev = 0;
    imgStruct.time_sleep = atoi(argv[2]);
    imgStruct.face_detected = 0;
    imgStruct.face_detect_enable = false;
    imgStruct.cap = new VideoCapture(imgStruct.dev);
    userInput = 0;
    endProgram = 0;


    //-------------------------------------------------------
    // Facial recognition setup
    //-------------------------------------------------------
    std::string cascadeName = "xml/haarcascade_frontalface_alt.xml";
    std::string nestedCascadeName = "xml/haarcascade_eye_tree_eyeglasses.xml";
    imgStruct.nestedCascade.load(samples::findFileOrKeep(nestedCascadeName));
    imgStruct.cascade.load(samples::findFile(cascadeName));

    std::cout << "Time Sleep: " << imgStruct.time_sleep << std::endl;


    //Listening
    listen(localSocket , 3);
    std::cout <<  "Waiting for connections...\n"
              <<  "Server Port:" << port << std::endl;

    // Initialize video structure Img and ImgGrey instances
    setup_img(&imgStruct);
    // Create thread to capture image frames
    pthread_create(&camera_processor_tid, NULL, capture_video, &imgStruct);
    pthread_create(&user_input_tid, NULL,  get_user_input, &imgStruct);

    //accept connection from an incoming client
    SLIST_HEAD(slisthead, VideoStreamStruct) head;
    SLIST_INIT(&head);
    VideoStream *videoStreamPtr;
    int remoteSocket;

    // Utilize polling to check for incoming client connection
    // Supports user input to gracefully terminate program execution without using SIGTERM
    struct pollfd pfds[1];
    pfds[0].fd = localSocket;
    pfds[0].events = POLLIN;

    while(endProgram == 0)
    {
	    int num_events = poll(pfds, 1, 2500);
	    if (num_events == 0)
	    {
	        printf("Poll timed out!\n");
	    }
	    else
	    {
	        int pollin_happened = pfds[0].revents & POLLIN;
	        if (pollin_happened)
		{
	            printf("File descriptor %d is ready to read\n", pfds[0].fd);
		    remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr, (socklen_t*)&addrLen);

	            if (remoteSocket < 0)
		    {
			    perror("No data on port...");
		    }
		    else
		    {
			    // Conenction accepted. Create new display thread to handle connection
			    std::cout << "Socket Connection accepted" << std::endl;
			    videoStreamPtr = new VideoStream;
			    videoStreamPtr->remoteSocket = remoteSocket;
			    std::cout << "newVideoStream->remoteSocket: " << videoStreamPtr->remoteSocket << std::endl;
			    videoStreamPtr->imgStruct = &imgStruct;
			    SLIST_INSERT_HEAD(&head, videoStreamPtr, entries);
			    pthread_create(&videoStreamPtr->thread_id, NULL, display, videoStreamPtr);
		    }

	        }
		else
		{
		    printf("Unexpected event occurred: %d\n", pfds[0].revents);
		}
	    }
    }

    std::cout << "Joining main processing threads..." << std::endl;
    pthread_join(camera_processor_tid,NULL);
    pthread_join(user_input_tid,NULL);
    std::cout << "Joining client display threads..." << std::endl;
    while(!SLIST_EMPTY(&head))
    {
	videoStreamPtr = SLIST_FIRST(&head);
	std::cout << "  - Joining thread_id: " << videoStreamPtr->thread_id << std::endl;
	pthread_join(videoStreamPtr->thread_id, NULL);
	SLIST_REMOVE_HEAD(&head, entries);
    }
    close(localSocket);

    return 0;
}

// Innitialize Img members of the ImgCaptureStruct
void setup_img(ImgCaptureStruct *imgStruct)
{
    // Initialize img object
    imgStruct->img = Mat::zeros(480 , 640, CV_8UC1);
    // Calculate image size
    imgStruct->imgSize = imgStruct->img.total() * imgStruct->img.elemSize();

    //make img continuos
    if ( !imgStruct->img.isContinuous() ) 
    {
          imgStruct->img = imgStruct->img.clone();
          imgStruct->imgGray = imgStruct->img.clone();
    }

    std::cout << "Image Setup Complete" << std::endl <<
		 "Image Size: " << imgStruct->imgSize << std::endl;
}


void *get_user_input(void *ptr)
{

	ImgCaptureStruct *imgStruct = (ImgCaptureStruct*) ptr;
	while (endProgram == 0)
	{
		std::cin >> userInput;
		if (userInput == 't')
		{
			imgStruct->face_detect_enable = !imgStruct->face_detect_enable;
			userInput = 0;
		}
		else if (userInput == 'q')
		{
			std::cout << "Ending Program" << std::endl;
			endProgram = 1;
		}
	}
	std::cout << "Terminating UI Thread" << std::endl; 
}



// Thread function to write frames from /dev/video# to the ImgCaptureStruct img members
// Logitech C270 webcam operates at max frame rate of 30 FPS
void *capture_video(void *ptr)
{
    // Obtain video structure attributes
    ImgCaptureStruct *imgStruct = (ImgCaptureStruct*) ptr;
    while(endProgram == 0)
    {
	// Capture frames at 3x logitech C270 frame rate (i.e. ~(30 FPS * 3))
	usleep(imgStruct->time_sleep);
	// Capture frame
	*(imgStruct->cap) >> imgStruct->img;
	// Convert to greyscale

	if (imgStruct->face_detect_enable)
		detectAndDraw(imgStruct->img, imgStruct->cascade, imgStruct->nestedCascade, &imgStruct->face_detected);
	cvtColor(imgStruct->img, imgStruct->imgGray, CV_BGR2GRAY);

//	cv:waitKey(1);
//      imshow("results", imgStruct->imgGray);
	imgStruct->face_detected = 0;
    }
    std::cout << "Terminating Video Capture Thread" << std::endl; 
//    cv::destroyAllWindows();
}

// Thread function to send img
void *display(void *ptr)
{
    int bytes = 0;
    VideoStream *vStream = (VideoStream*) ptr;
    int socket = vStream->remoteSocket;
    std::cout << "Innitialized display thread ID: " << vStream->thread_id << std::endl;
    while(endProgram == 0)
    {
	usleep(vStream->imgStruct->time_sleep);
        //send current frame
	if ((bytes = send(socket, vStream->imgStruct->imgGray.data, vStream->imgStruct->imgSize, 0)) < 0)
	{
            std::cerr << "bytes = " << bytes << std::endl;
            break;
        }
	else
	{
//	    std::cout << "Bytes Sent: " << bytes << std::endl;
	}
    }
    std::cout << "Terminating Display for Thread ID: " << vStream->thread_id << std::endl;
}
