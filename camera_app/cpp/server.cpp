/**************************************************************************************************
* @file        server.cpp
* @version     0.1.1
* @type:       OpenCV Video streaming socket application function implementation and 
*			   main application
* @brief       OpenCV application that provides video stream to client requests.
*		 		  - Main Thread: Generate video stream and write image data to a global structure
* 				  - Innitializes a socket connection to support client requests for video stream
* 				  - Each client request is handled by a separate thread
* @author      Julian Abbott-Whitley (julian.abbott-whitley@Colorado.edu)
* @license:    GNU GPLv3   (attached below)
*
* @references: The following sources were referenced during development
*					- [OpenCV](https://github.com/opencv/)
*					- [OpenCV video streaming over TCP/IP (C++)](https://gist.github.com/Tryptich/2a15909e384b582c51b5)
*					- [Video Streaming using Python and Flask](https://github.com/log0/video_streaming_with_flask_example)
*					- [Python Socket Communication](https://stackoverflow.com/questions/30988033/sending-live-video-frame-over-network-in-python-opencv)
*					- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/#a-simple-stream-server)
*					- [Read, Write and Display a video using OpenCV](https://learnopencv.com/read-write-and-display-a-video-using-opencv-cpp-python/)
*					- [C++ videocapture-frames-through-socket](https://stackoverflow.com/questions/47881656/how-to-transfer-cvvideocapture-frames-through-socket-in-client-server-model-o)
*					- [Broken-down-time](https://zetcode.com/articles/cdatetime/)
**************************************************************************************************/

//#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <syslog.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <sys/time.h>
#include <time.h>
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

	// MAIN processing threads
    pthread_t camera_processor_tid;
	pthread_t camera_recording_tid;

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
    imgStruct.dev = 0;												// Camera device
	imgStruct.frame_rate = 30.0;									// Default Frame Rate: ~30 FPS (Logitech C270 max frame rate = 30 FPS
    imgStruct.time_sleep = (1 / imgStruct.frame_rate) * 1000000;	// Time to sleep in microseconds between frame captures
    imgStruct.face_detected = 0;									// Assume no face detected innitially 
    imgStruct.face_detect_enable = false;							// Enable face detection as default
    imgStruct.pauseVideo = false;									// Pause Default = false
	imgStruct.record_time = 10;						     			// Default = 10 seconds for testing purposes
	imgStruct.manual_record = false;
	imgStruct.dir_name_size = 256;
	imgStruct.write_dir = (char*)malloc(imgStruct.dir_name_size);

    imgStruct.cap = new VideoCapture(imgStruct.dev);
    END_PROGRAM = 0;

	DEBUG_LOG("Frame Rate: %.2f/s", imgStruct.frame_rate);
	DEBUG_LOG("Time Sleep: %.3f us", imgStruct.time_sleep);

    //-------------------------------------------------------
    // Facial recognition setup
    //-------------------------------------------------------
	const char *dev_arch = "x86_64";
	dev_arch = getBuild();
	if (dev_arch == "x86_64")
	{
		// Development machine, use relative path
	    imgStruct.nestedCascade.load(samples::findFileOrKeep("xml/haarcascade_eye_tree_eyeglasses.xml"));
	    imgStruct.cascade.load(samples::findFile("xml/haarcascade_frontalface_alt.xml"));
	}
	else
	{
	    // Target machine, use full path
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
	pthread_create(&camera_recording_tid, NULL, record_video, &imgStruct);
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
    while(END_PROGRAM == 0)
    {
		TRACE_LOG("TOP OF MAIN WHILE LOOP");
	    int num_events = poll(pfds, 1, 2500);	// Poll for 2500 ms
	    if (num_events == 0)
	    {
	        TRACE_LOG("Poll timed out, join stagnant threads");
		    VideoStream *videoStreamPtr_TMP;
		    SLIST_FOREACH_SAFE(videoStreamPtr, &head, entries, videoStreamPtr_TMP)
		    {
			    TRACE_LOG("VideoStream THREAD ID [%ld] STATUS COMPLETE = %d", videoStreamPtr->thread_id, videoStreamPtr->thread_complete);
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
			TRACE_LOG("CLIENT CONNECTION DETECTED");
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
		TRACE_LOG("END OF MAIN WHILE LOOP\n");
    }

    // Cleanup
    DEBUG_LOG("Joining MAIN processing threads...");
    DEBUG_LOG("Joining Camera Processor thread: [%ld]", camera_processor_tid);
    pthread_join(camera_processor_tid, NULL);
    DEBUG_LOG("Joining Camera Recording thread: [%ld]", camera_recording_tid);
	pthread_join(camera_recording_tid, NULL);
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

// Sets the local time into the provide char*
// Return 0 on success and -1 on failure
int get_local_time(char* tm_str, int buf_size)
{
    time_t rawtime = time(NULL);
    if (rawtime == -1)
	{
        syslog(LOG_DEBUG, "time() function failed");
		snprintf(tm_str, buf_size, "%02d:%02d:%02d\n",0,0,0);
		return -1;
    }
    struct tm *ptm = localtime(&rawtime);
    if (ptm == NULL)
	{
        syslog(LOG_DEBUG, "localtime() function failed");
		snprintf(tm_str, buf_size, "%02d:%02d:%02d\n",0,0,0);
		return -1;
    }
	snprintf(tm_str, buf_size, "%02d:%02d:%02d\n", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	return 0;
}

// Thread function to write frames from /dev/video# to the ImgCaptureStruct img members
// Logitech C270 webcam operates at max frame rate of 30 FPS
void *capture_video(void *ptr)
{
    // Obtain video structure attributes
    ImgCaptureStruct *imgStruct = (ImgCaptureStruct*) ptr;
	float m = 0.75;						// Scale factor for text on images
	int rows;							// Used to place text at specific row on images
	char facedetect_str[100];			// FACEDETECT: ENABLE/DISABLE print str
	char timer_Str[100];				// TIMER print string
	int buf_size = 9;					// tm_str buff size
	char* tm_str;						// Time stamp in local time (must be freed at end of function)
	tm_str = (char*)malloc(buf_size);	// Allocate memory for tm_str
	// Loop until program terminated

	struct timeval t1, t2;
	double record_time_left;
    while(END_PROGRAM == 0)
    {
		// Max frame rate of the Logitech C270 is 30 FPS
		// No need to capture any faster that, sleep based on the current specified frame rate
		// Time sleep calculated when frame rate is set by the user
		usleep(imgStruct->time_sleep);
		// Capture frame
		if (imgStruct->pauseVideo == 0)
		{
			// Make sure camera is still connected
			if ( !imgStruct->cap->isOpened() )
				DEBUG_LOG("%s", "Camera device not available");
			else
			{
				// Start video capture 
				*(imgStruct->cap) >> imgStruct->img;
				// If face dectection is enabled
				if (imgStruct->face_detect_enable)
				{
					// Analyze current frame for a persons face
					detectAndDraw(imgStruct->img, imgStruct->cascade, imgStruct->nestedCascade, &imgStruct->face_detected);
					// Update string to show status of face detection settings
					strcpy(facedetect_str, "FACE DETECTECTION: ENABLED");
					// If face detected in frame
					if(imgStruct->face_detected)
					{
						// Set "RECORD_VIDEO" flag and start the count down timer
						RECORD_VIDEO = true;
						timer_init(imgStruct->record_time);
						gettimeofday(&t1, NULL);
					}
				}
				else
				{
					// Update string to show status of face detection settings
					strcpy(facedetect_str, "FACE DETECTECTION: DISABLED");
				}
				// Convert image to greyscale
				cvtColor(imgStruct->img, imgStruct->imgGray, CV_BGR2GRAY);
				rows = imgStruct->imgGray.rows;

				// Add program settings and time stamps to image

				// Get and print time stamp
				get_local_time(tm_str, buf_size);
				cv::putText(imgStruct->imgGray, tm_str, cv::Point(10, rows - (rows / 10)),
		            				cv::FONT_HERSHEY_SIMPLEX, m, CV_RGB(255, 0, 0), 2);
				// Print facedetect enable status
				cv::putText(imgStruct->imgGray, facedetect_str, cv::Point(10, rows - (rows / 40)),
							cv::FONT_HERSHEY_SIMPLEX, m, CV_RGB(255, 0, 0), 2);

				if (RECORD_VIDEO & (imgStruct->manual_record == false))
				{
					// Capturing video due to face detection 
					gettimeofday(&t2, NULL);
					record_time_left = imgStruct->record_time - (t2.tv_sec - t1.tv_sec);
					snprintf(timer_Str, sizeof(timer_Str), "MODE [FD]: TIMER: %.0fs", record_time_left);
					cv::putText(imgStruct->imgGray, "RECORDING", cv::Point(10, (rows / 12)),
		            				cv::FONT_HERSHEY_SIMPLEX, m, CV_RGB(255, 0, 0), 2);
					cv::putText(imgStruct->imgGray, timer_Str, cv::Point(10, (rows / 7)),
							cv::FONT_HERSHEY_SIMPLEX, m, CV_RGB(255, 0, 0), 2);
				}
				else if (RECORD_VIDEO)
				{
					// Capturing video manually 
					snprintf(timer_Str, sizeof(timer_Str), "MODE [MANUAL]");
					cv::putText(imgStruct->imgGray, "RECORDING", cv::Point(10, (rows / 12)),
		            				cv::FONT_HERSHEY_SIMPLEX, m, CV_RGB(255, 0, 0), 2);
					cv::putText(imgStruct->imgGray, timer_Str, cv::Point(10, (rows / 7)),
							cv::FONT_HERSHEY_SIMPLEX, m, CV_RGB(255, 0, 0), 2);
				}
			}
		}
		imgStruct->face_detected = 0;
	} // End while loop
	free(tm_str);
    DEBUG_LOG("Terminating Video Capture Thread");
}


// Thread to manage video recording
void *record_video(void *ptr)
{
    // Obtain video structure attributes
    ImgCaptureStruct *imgStruct = (ImgCaptureStruct*) ptr;

	int buf_size = 9;					// tm_str buff size
	char* tm_str;						// Time stamp in local time (must be freed at end of function)
	tm_str = (char*)malloc(buf_size);
	VideoWriter vWriter;
	Size S = Size((int) 640,(int) 480);
	// Create a new video file to write to
    get_local_time(tm_str, buf_size);
	snprintf(imgStruct->write_dir, imgStruct->dir_name_size, "/tmp/video_recording_%s.avi", tm_str);
	vWriter.open(imgStruct->write_dir, CV_FOURCC('M','J','P','G'), imgStruct->frame_rate, S, 0);
	long frames = 0;
	while (END_PROGRAM == 0)
	{
		usleep(imgStruct->time_sleep);
		if (RECORD_VIDEO)
		{
			// Capture frame
			vWriter << imgStruct->imgGray;
		}
	}
	vWriter.release();
	DEBUG_LOG("Video recording complete");
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

    while(END_PROGRAM == 0)
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
						vStream->imgStruct->manual_record = false;
						break;
					// Pause video
					case 200 :
						vStream->imgStruct->pauseVideo = !vStream->imgStruct->pauseVideo;
						break;
					// Record video
					case 300 :
						RECORD_VIDEO = !RECORD_VIDEO;
						vStream->imgStruct->manual_record = !vStream->imgStruct->manual_record;
						vStream->imgStruct->face_detect_enable = false;
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


