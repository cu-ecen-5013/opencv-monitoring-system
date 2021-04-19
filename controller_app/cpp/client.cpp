  
/**************************************************************************************************
* @file        client.cpp
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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace cv;


int main(int argc, char** argv)
{

    //--------------------------------------------------------
    //networking stuff: socket , connect
    //--------------------------------------------------------
    int         sokt;
    char*       serverIP;
    int         serverPort;

    if (argc < 3) {
           std::cerr << "Usage: cv_video_cli <serverIP> <serverPort> " << std::endl;
    }

    serverIP   = argv[1];
    serverPort = atoi(argv[2]);

    struct  sockaddr_in serverAddr;
    socklen_t           addrLen = sizeof(struct sockaddr_in);

    if ((sokt = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "socket() failed" << std::endl;
    }

    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, serverIP, &(serverAddr.sin_addr));
    serverAddr.sin_port = htons(serverPort);

    if (connect(sokt, (sockaddr*)&serverAddr, addrLen) < 0) {
        std::cerr << "connect() failed!" << std::endl;
    }



    //----------------------------------------------------------
    //OpenCV Code
    //----------------------------------------------------------

    Mat img;
    img = Mat::zeros(480 , 640, CV_8UC1);
    int imgSize = img.total() * img.elemSize();
    uchar *iptr = img.data;
    int bytes = 0;
    int key;
    Size S = Size((int) 640,(int) 480);
    //make img continuos
    if ( ! img.isContinuous() ) {
          img = img.clone();
    }

    int frameRate = atoi(argv[3]);

    std::cout << "Image Size:" << imgSize << std::endl;
    VideoWriter video;
    video.open("outcpp.avi", CV_FOURCC('M','J','P','G'), frameRate, S, 0);

    int fCount = 0;
    while (fCount < 100) {

        if ((bytes = recv(sokt, iptr, imgSize , MSG_WAITALL)) == -1)
	{
            std::cerr << "recv failed, received bytes = " << bytes << std::endl;
        }
	video << img;
 	fCount++;
    }

    video.release();
    close(sokt);
    return 0;
}
