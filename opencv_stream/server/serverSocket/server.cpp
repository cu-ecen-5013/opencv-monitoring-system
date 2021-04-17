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

using namespace cv;

typedef struct
{
	int dev;
	int imgSize;
	Mat img;
	Mat imgGray;
	VideoCapture *cap; // open the default camera
} ImgCaptureStruct;


typedef struct
{
	int remoteSocket;
	ImgCaptureStruct *imgStruct;
} VideoStream;

void *display(void *);
void *capture_video(void *);
void setup_img(ImgCaptureStruct *);

int main(int argc, char** argv)
{

    //--------------------------------------------------------
    //networking stuff: socket, bind, listen
    //--------------------------------------------------------
    int localSocket;
    int remoteSocket;
    int port;

    ImgCaptureStruct imgStruct;
    imgStruct.dev = 0;
    imgStruct.cap = new VideoCapture(imgStruct.dev);

    port = *argv[1];

    struct  sockaddr_in localAddr,
                        remoteAddr;

    pthread_t camera_processor_tid;
    pthread_t thread_id;
    int addrLen = sizeof(struct sockaddr_in);

    if ( (argc > 1) && (strcmp(argv[1],"-h") == 0) ) {
          std::cerr << "usage: ./cv_video_srv [port] [capture device]\n" <<
                       "port           : socket port (4097 default)\n" <<
                       "capture device : (0 default)\n" << std::endl;

          exit(1);
    }


    if (argc == 2) port = atoi(argv[1]);

    localSocket = socket(AF_INET , SOCK_STREAM , 0);
    if (localSocket == -1){
         perror("socket() call failed!!");
    }

    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons( port );

    if( bind(localSocket,(struct sockaddr *)&localAddr , sizeof(localAddr)) < 0) {
         perror("Can't bind() socket");
         exit(1);
    }

    //Listening
    listen(localSocket , 3);
    std::cout <<  "Waiting for connections...\n"
              <<  "Server Port:" << port << std::endl;

    // Initialize video structure Img and ImgGrey instances
    setup_img(&imgStruct);
    // Create thread to capture image frames
    pthread_create(&camera_processor_tid, NULL, capture_video, &imgStruct);

    //accept connection from an incoming client
    while(1){
    //if (remoteSocket < 0) {
    //    perror("accept failed!");
    //    exit(1);
    //}
     VideoStream *newVideoStream = new VideoStream;
     newVideoStream->remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr, (socklen_t*)&addrLen);
     newVideoStream->imgStruct = &imgStruct;
      //std::cout << remoteSocket<< "32"<< std::endl;
    if (remoteSocket < 0) {
        perror("accept failed!");
        exit(1);
    }
    std::cout << "Connection accepted" << std::endl;
//    pthread_create(&thread_id,NULL,display,&newSocket->remoteSocket);
    pthread_create(&thread_id,NULL,display,newVideoStream);

     //pthread_join(thread_id,NULL);

    }
    //pthread_join(thread_id,NULL);
    //close(remoteSocket);

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

// Threa function to write frames from /dev/video# to the ImgCaptureStruct img members
void *capture_video(void *ptr)
{
    // Obtain video structure attributes
    ImgCaptureStruct *imgStruct = (ImgCaptureStruct*) ptr;
    while(1)
    {
	// Capture frame
	*(imgStruct->cap) >> imgStruct->img;
	// Convert Color to grey
	cvtColor(imgStruct->img, imgStruct->imgGray, CV_BGR2GRAY);
    }
}

// Thread function to send img
void *display(void *ptr)
{
    int bytes = 0;
    VideoStream *vStream = (VideoStream*) ptr;
    int socket = vStream->remoteSocket;

    while(1)
    {
	usleep(1000);
        //send current frame
	if ((bytes = send(socket, vStream->imgStruct->imgGray.data, vStream->imgStruct->imgSize, 0)) < 0)
	{
            std::cerr << "bytes = " << bytes << std::endl;
            break;
        }
	else
	{
	    std::cout << "Bytes Sent: " << bytes << std::endl;
	}
    }

return 0;
}
