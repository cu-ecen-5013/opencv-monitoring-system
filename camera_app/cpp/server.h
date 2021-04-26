/**************************************************************************************************
* @file        server.h
* @version     0.1.1
* @type:       OpenCV Video streaming socket application function headers
* @brief       OpenCV application that provides video stream to client requests.

* @author      Julian Abbott-Whitley (julian.abbott-whitley@Colorado.edu)
* @license:    GNU GPLv3   (attached below)
*
* @references: The following sources were referenced during development
*              https://stackoverflow.com/questions/152016/detecting-cpu-architecture-compile-time
*
**************************************************************************************************/

#include "opencv2/opencv.hpp"
#include "queue.h"

using namespace cv;

//#define DEBUG_LOG(...)
#define DEBUG_LOG(msg,...) printf("[ DEBUG ] " msg "\n", ##__VA_ARGS__)
//#define DEBUG_LOG(msg,...) syslog(LOG_DEBUG, msg "\n", ##__VA_ARGS__)

#define TRACE_LOG(...)
//#define TRACE_LOG(msg,...) printf("[ TRACE ] " msg "\n", ##__VA_ARGS__)


typedef struct
{
	int dev;                    // Camera device
	int imgSize;                // Total size of image in bytes
	float frame_rate;           // Frame rate to capture images
	float time_sleep;           // Time to sleep in micro Seconds between each frame capture
	int face_detected;          // Face detected flag
	bool face_detect_enable;    // boolean value to toggle face detection
	bool pauseVideo;            // boolean to pause video feed
	int record_time;			// Amount of time to record video after a face is detected
	bool manual_record;
	char *write_dir;
	int dir_name_size;
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
	bool thread_complete;
	ImgCaptureStruct *imgStruct;
	SLIST_ENTRY(VideoStreamStruct) entries;
};

typedef VideoStreamStruct VideoStream;

void *display(void *);
void *capture_video(void *);
void *record_video(void *);
void setup_img(ImgCaptureStruct *);
int get_local_time(char*, int);


//
// Source: https://stackoverflow.com/questions/152016/detecting-cpu-architecture-compile-time
//
extern "C" {
    const char *getBuild() { //Get current architecture, detectx nearly every architecture. Coded by Freak
        #if defined(__x86_64__) || defined(_M_X64)
        return "x86_64";
        #elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
        return "x86_32";
        #elif defined(__ARM_ARCH_2__)
        return "ARM2";
        #elif defined(__ARM_ARCH_3__) || defined(__ARM_ARCH_3M__)
        return "ARM3";
        #elif defined(__ARM_ARCH_4T__) || defined(__TARGET_ARM_4T)
        return "ARM4T";
        #elif defined(__ARM_ARCH_5_) || defined(__ARM_ARCH_5E_)
        return "ARM5"
        #elif defined(__ARM_ARCH_6T2_) || defined(__ARM_ARCH_6T2_)
        return "ARM6T2";
        #elif defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__)
        return "ARM6";
        #elif defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
        return "ARM7";
        #elif defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
        return "ARM7A";
        #elif defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
        return "ARM7R";
        #elif defined(__ARM_ARCH_7M__)
        return "ARM7M";
        #elif defined(__ARM_ARCH_7S__)
        return "ARM7S";
        #elif defined(__aarch64__) || defined(_M_ARM64)
        return "ARM64";
        #elif defined(mips) || defined(__mips__) || defined(__mips)
        return "MIPS";
        #elif defined(__sh__)
        return "SUPERH";
        #elif defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || defined(__POWERPC__) || defined(__ppc__) || defined(__PPC__) || defined(_ARCH_PPC)
        return "POWERPC";
        #elif defined(__PPC64__) || defined(__ppc64__) || defined(_ARCH_PPC64)
        return "POWERPC64";
        #elif defined(__sparc__) || defined(__sparc)
        return "SPARC";
        #elif defined(__m68k__)
        return "M68K";
        #else
        return "UNKNOWN";
        #endif
    }
}

