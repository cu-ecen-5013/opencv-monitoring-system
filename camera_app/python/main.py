# main.py
# import the necessary packages
from flask import Flask, render_template, Response, request
from camera import VideoCamera
import socket
import os
import threading

app = Flask(__name__)

# Configure HTML Web page ip and port settings
# Get wlan0 ip address of 'this' device
gw = os.popen("ip -4 route show default").read().split()
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.connect((gw[2], 0))
HTML_IPADDR = s.getsockname()[0]
HTML_PORT='5000'
s.close()

userInput = 0

# localhost config
# Used to capturing C++ video stream over socket IPC
LOCAL_HOST = '127.0.0.1'
LOCAL_PORT = 4099


@app.route("/",methods=["POST","GET"])
def index():
	global userInput
	ui = {
		1:"FaceDetect",
		2:"PauseVideo",
		3:"RecordVideo",
		4:"FrameRateOptions"
	}
	if request.method == "POST":
		for k in ui.keys():
			userInput = request.form.get(ui[k])
			if(userInput):
				print("userINput: %s" % userInput)
				break
	return render_template('index.html')


def gen(vServer):
	global userInput
	while True:
		try:
			if (int(userInput) is not 0):
				vServer.send_data(userInput)
				userInput = 0
		except ValueError:
			print("text input error")
			userInput = 0
		#get camera frame
		frame = vServer.get_data()
		if (frame is not None):
			yield (b'--frame\r\n'
			       b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n\r\n')


@app.route('/video_feed', methods=['GET', 'POST'])
def video_feed():
    video_feed = VideoCamera(LOCAL_HOST, LOCAL_PORT)
    return Response(gen(video_feed),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

def button_response(button):
	return response[button]


if __name__ == '__main__':
    # defining server ip address and port
    app.run(host=HTML_IPADDR,port=HTML_PORT, debug=True)
