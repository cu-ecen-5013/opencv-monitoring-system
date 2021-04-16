 /**
 * OpenCV video streaming over TCP/IP
 * Server: Captures video from a webcam and send it to a client
 * by Isaac Maia
 */

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
} VideoStream;


typedef struct
{
	int remoteSocket;
	VideoStream *vidStream;

} SocketStream;

void *display(void *);
void *capture_video(void *);
void setup_img(VideoStream *);

int main(int argc, char** argv)
{

    //--------------------------------------------------------
    //networking stuff: socket, bind, listen
    //--------------------------------------------------------
    int localSocket;
    int remoteSocket;
    int port;

    VideoStream vidStream;
    vidStream.dev = 0;
    vidStream.cap = new VideoCapture(vidStream.dev);

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
    setup_img(&vidStream);
    // Create thread to capture image frames
    pthread_create(&camera_processor_tid, NULL, capture_video, &vidStream);

    //accept connection from an incoming client
    while(1){
    //if (remoteSocket < 0) {
    //    perror("accept failed!");
    //    exit(1);
    //}
     SocketStream *newSocket = new SocketStream;
     newSocket->remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr, (socklen_t*)&addrLen);
     newSocket->vidStream = &vidStream;
      //std::cout << remoteSocket<< "32"<< std::endl;
    if (remoteSocket < 0) {
        perror("accept failed!");
        exit(1);
    }
    std::cout << "Connection accepted" << std::endl;
//    pthread_create(&thread_id,NULL,display,&newSocket->remoteSocket);
    pthread_create(&thread_id,NULL,display,newSocket);

     //pthread_join(thread_id,NULL);

    }
    //pthread_join(thread_id,NULL);
    //close(remoteSocket);

    return 0;
}


void setup_img(VideoStream *vStream)
{
    // Initialize img object
    vStream->img = Mat::zeros(480 , 640, CV_8UC1);
    // Calculate image size

    if (!vStream->img.isContinuous())
    {
	vStream->img = vStream->img.clone();
    }


    vStream->imgSize = vStream->img.total() * vStream->img.elemSize();

    //make img continuos
    if ( !vStream->img.isContinuous() ) 
    {
          vStream->img = vStream->img.clone();
          vStream->imgGray = vStream->img.clone();
    }

    std::cout << "Image Setup Complete" << std::endl <<
		 "Image Size: " << vStream->imgSize << std::endl;
}

void *capture_video(void *ptr)
{
    // Obtain video structure attributes
    VideoStream *vStream = (VideoStream*) ptr;
    while(1)
    {
	// Capture frame
	*(vStream->cap) >> vStream->img;
	// Convert Color to grey
	cvtColor(vStream->img, vStream->imgGray, CV_BGR2GRAY);
    }
}

void *display(void *ptr)
{
    int bytes = 0;
    SocketStream *sStream = (SocketStream*) ptr;
    int socket = sStream->remoteSocket;

    while(1)
    {
                //send current frame
		if ((bytes = send(socket, sStream->vidStream->imgGray.data, sStream->vidStream->imgSize, 0)) < 0){
                     std::cerr << "bytes = " << bytes << std::endl;
                     break;
                } else {
		     std::cout << "Bytes Sent: " << bytes << std::endl;
		}
    }

return 0;
}
