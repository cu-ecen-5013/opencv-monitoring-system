/**************************************************************************************************
* @file        server.cpp
* @version     0.1.1
* @type:       OpenCV Video streaming socket application function implementation and 
			   main application
* @brief       OpenCV application that provides video stream to client requests.
		 		  - Main Thread: Generate video stream and write image data to a global structure
 				  - Innitializes a socket connection to support client requests for video stream
 				  - Each client request is handled by a separate thread
* @author      Julian Abbott-Whitley (julian.abbott-whitley@Colorado.edu)
* @license:    GNU GPLv3   (attached below)
*
* @references: The following sources were referenced during development
					- [OpenCV](https://github.com/opencv/)
					- [OpenCV video streaming over TCP/IP (C++)](https://gist.github.com/Tryptich/2a15909e384b582c51b5)
					- [Video Streaming using Python and Flask](https://github.com/log0/video_streaming_with_flask_example)
					- [Python Socket Communication](https://stackoverflow.com/questions/30988033/sending-live-video-frame-over-network-in-python-opencv)
					- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/#a-simple-stream-server)
					- [Read, Write and Display a video using OpenCV](https://learnopencv.com/read-write-and-display-a-video-using-opencv-cpp-python/)
					- [C++ videocapture-frames-through-socket](https://stackoverflow.com/questions/47881656/how-to-transfer-cvvideocapture-frames-through-socket-in-client-server-model-o)

*
**************************************************************************************************/

#include <iostream>
#include <sys/socket.h>
#include <syslog.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include "server.h"
#include "facedetect.h"
#include "camera_app_signals.h" 


using namespace cv;

int main(int argc, char** argv)
{

	syslog(LOG_DEBUG, "Starting OPENCV server");
	// Initialize signal handlers
    init_sigHandlers();

    //--------------------------------------------------------
    // Setup network configuration settings: socket, bind, listen
    //--------------------------------------------------------
    int localSocket;
    int port = 4099;	// Default port 4099

    struct  sockaddr_in localAddr,
                        remoteAddr;

    pthread_t camera_processor_tid;
    int addrLen = sizeof(struct sockaddr_in);

    localSocket = socket(AF_INET , SOCK_STREAM , 0);
    if (localSocket == -1){
         syslog(LOG_DEBUG, "Failed to innitialize socket(AF_INET, SOCK_STREAM)");
    }

    int enable = 1;
    if (setsockopt(localSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        syslog(LOG_DEBUG, "setsockopt(SO_REUSEADDR) failed");

    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(4099);

    if( bind(localSocket,(struct sockaddr *)&localAddr , sizeof(localAddr)) < 0) {
         syslog(LOG_DEBUG, "Can't bind() socket");
         exit(1);
    }

    //-------------------------------------------------------
    // Innitialize ImgCaptureStruct members 
    //-------------------------------------------------------
    ImgCaptureStruct imgStruct;
    imgStruct.dev = 0;
	imgStruct.frame_rate = 30.0;									// Default Frame Rate: ~30 FPS (Logitech C270 max frame rate = 30 FPS
    imgStruct.time_sleep = (1 / imgStruct.frame_rate) * 1000000;
    imgStruct.face_detected = 0;
    imgStruct.face_detect_enable = false;
    imgStruct.pauseVideo = false;
    imgStruct.cap = new VideoCapture(imgStruct.dev);
    endProgram = 0;

	DEBUG_LOG("Frame Rate: %.2f/s", imgStruct.frame_rate);
	DEBUG_LOG("Time Sleep: %.3f us", imgStruct.time_sleep);

    //-------------------------------------------------------
    // Facial recognition setup
    //-------------------------------------------------------
	const char *dev_arch = "x86_64";
	dev_arch = getBuild();
	if (dev_arch == "x86_64")
	{
	    imgStruct.nestedCascade.load(samples::findFileOrKeep("xml/haarcascade_eye_tree_eyeglasses.xml"));
	    imgStruct.cascade.load(samples::findFile("xml/haarcascade_frontalface_alt.xml"));

	}
	else
	{
	    imgStruct.nestedCascade.load(samples::findFileOrKeep("/usr/bin/opencv/camera_app/cpp/xml/haarcascade_eye_tree_eyeglasses.xml"));
	    imgStruct.cascade.load(samples::findFile("/usr/bin/opencv/camera_app/cpp/xml/haarcascade_frontalface_alt.xml"));
	}

    // Listen, output status to both syslog and debug log
    listen(localSocket , 3);
    syslog(LOG_DEBUG, "Server Listening on Port: %d", port);

    // Initialize video structure Img and ImgGrey instances
    setup_img(&imgStruct);
    // Create thread to capture image frames
    pthread_create(&camera_processor_tid, NULL, capture_video, &imgStruct);

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


	// Infinite server loop
	// Poll for client request
	// If poll times out
	//		- loop through linked list of open threads / existing client connections 
	//		- join thread if thread is complete (i.e. client closed connection)
	// Else
	//		- Handle incoming client request with new thread
    while(endProgram == 0)
    {
		DEBUG_LOG("TOP OF MAIN WHILE LOOP");
	    int num_events = poll(pfds, 1, 2500);	// Poll for 2500 ms
	    if (num_events == 0)
	    {
	        DEBUG_LOG("Poll timed out, join stagnant threads");
		    VideoStream *videoStreamPtr_TMP;
		    SLIST_FOREACH_SAFE(videoStreamPtr, &head, entries, videoStreamPtr_TMP)
		    {
			    DEBUG_LOG("VideoStream THREAD ID [%ld] STATUS COMPLETE = %d", videoStreamPtr->thread_id, videoStreamPtr->thread_complete);
		    	if (videoStreamPtr->thread_complete == true)
				{
			    	DEBUG_LOG("Joining thread: [%ld]", videoStreamPtr->thread_id);
			    	pthread_join(videoStreamPtr->thread_id, NULL);
			    	SLIST_REMOVE(&head, videoStreamPtr, VideoStreamStruct, entries);
			    	free(videoStreamPtr);
		    	}
		    }
	    }
	    else
	    {
			DEBUG_LOG("CLIENT CONNECTION DETECTED");
	        int pollin_happened = pfds[0].revents & POLLIN;
	        if (pollin_happened)
			{
	            DEBUG_LOG("File descriptor %d is ready to read\n", pfds[0].fd);
		    	remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr, (socklen_t*)&addrLen);
	            if (remoteSocket < 0)
		    	{
			    	DEBUG_LOG("ERROR ACCEPTING SOCKET CONNECTION");
		    	}
		    	else
		    	{
			    	// Conenction accepted. Create new display thread to handle connection
				    DEBUG_LOG("Socket Connection accepted");
				    videoStreamPtr = new VideoStream;
				    videoStreamPtr->remoteSocket = remoteSocket;
				    DEBUG_LOG("newVideoStream->remoteSocket: %d", videoStreamPtr->remoteSocket);
				    videoStreamPtr->imgStruct = &imgStruct;
				    SLIST_INSERT_HEAD(&head, videoStreamPtr, entries);
				    pthread_create(&videoStreamPtr->thread_id, NULL, display, videoStreamPtr);
		    	}

			}
			else
			{
		    	syslog(LOG_DEBUG, "Unexpected event occurred: %d\n", pfds[0].revents);
			}
	    }
		DEBUG_LOG("END OF MAIN WHILE LOOP\n");
    }

    // Cleanup
    DEBUG_LOG("Joining MAIN processing threads...");
    DEBUG_LOG("Joining Camera Processor thread: [%ld]", camera_processor_tid);
    pthread_join(camera_processor_tid, NULL);
    DEBUG_LOG("Joining client display threads...");
    while(!SLIST_EMPTY(&head))
    {
	videoStreamPtr = SLIST_FIRST(&head);
	DEBUG_LOG("    - Joining thread: [%ld]", videoStreamPtr->thread_id);
	pthread_join(videoStreamPtr->thread_id, NULL);
	free(videoStreamPtr);
	SLIST_REMOVE_HEAD(&head, entries);
    }
    close(localSocket);
    syslog(LOG_DEBUG, "Closing OPENCV server");
    DEBUG_LOG("Ending MAIN: MAIN Thread ID [%ld]", pthread_self());
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

    DEBUG_LOG("Image Setup Complete");
    DEBUG_LOG("Image Size: %d", imgStruct->imgSize);
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
	if (imgStruct->pauseVideo == 0)
	{
		*(imgStruct->cap) >> imgStruct->img;
		// Convert to greyscale

		if (imgStruct->face_detect_enable)
			detectAndDraw(imgStruct->img, imgStruct->cascade, imgStruct->nestedCascade, &imgStruct->face_detected);
		cvtColor(imgStruct->img, imgStruct->imgGray, CV_BGR2GRAY);
	}
	imgStruct->face_detected = 0;
    }
    DEBUG_LOG("Terminating Video Capture Thread");
}

// Thread function to send img
void *display(void *ptr)
{
    int bytes = 0;
    char buf[10] = {'\0'};
    int userInput = 0;
    VideoStream *vStream = (VideoStream*) ptr;
    vStream->thread_complete = false;
    int socket = vStream->remoteSocket;
    DEBUG_LOG("Innitialized display thread ID: %ld", vStream->thread_id);

    struct pollfd pfds[1];
    pfds[0].fd = socket;
    pfds[0].events = POLLIN;

    while(endProgram == 0)
    {
	// Poll for 1000 ms
	int num_events = poll(pfds, 1, 30);
	if (num_events == 0)
	{
		// Poll timed out, do noting
	}
	else
	{
	    // Received data from client, handle input
	    int pollin_happened = pfds[0].revents & POLLIN;
	    if (pollin_happened)
	    {
			bytes = recv(socket, buf, 10, 0);
			if (bytes > 0)	// Handle bytes received from client
			{
				char* token = strtok(buf, "\n");
				strcpy(buf, token);
				userInput = atoi(buf);
				DEBUG_LOG("Data Received: %d", userInput);
				switch(userInput)
				{
					// Expected Default condition, do nothing
					case 0 :
						break;
					// Toggle face detection
					case 100 :
						vStream->imgStruct->face_detect_enable = !vStream->imgStruct->face_detect_enable;
						break;
					// Pause video
					case 200 :
						vStream->imgStruct->pauseVideo = !vStream->imgStruct->pauseVideo;
						break;
					// Record video
					case 300 :
						break;
					// Frame Rate adjustment
					default :
						// Get user input frame rate
						vStream->imgStruct->frame_rate = userInput;
						// Convert frame rate micro-useconds
						vStream->imgStruct->time_sleep = (1 / vStream->imgStruct->frame_rate) * 1000000;
						DEBUG_LOG("Adjusted Frame Rate: %.2f/s", vStream->imgStruct->frame_rate);
						DEBUG_LOG("Adjusted Time Sleep: %.2f us", vStream->imgStruct->time_sleep);
						break;
				}
				// Clear buf and reset userInput value to default;
				int i;
				for (i=0; i < 10; i++)
				{
					buf[i] = '\0';
				}
				userInput = 0;

			}	// End of Handle bytes received block
 	    }
		else // num_events > 0 but polling_happened = 0, something went wrong with poll event
	    {
			syslog(LOG_DEBUG, "Unexpected event occurred: %d\n", pfds[0].revents);
	    }
	}

    // Done handling client input, send current frame
	if ((bytes = send(socket, vStream->imgStruct->imgGray.data, vStream->imgStruct->imgSize, 0)) < 0)
	{
	       syslog(LOG_DEBUG, "Error sending data --> retVal = %d", bytes);
	       break;
	}
	else
	{
		// std::cout << "Bytes Sent: " << bytes << std::endl;
	}

    }
    vStream->thread_complete = true;
    DEBUG_LOG("Terminating Display for Thread ID: %ld", vStream->thread_id);
}


