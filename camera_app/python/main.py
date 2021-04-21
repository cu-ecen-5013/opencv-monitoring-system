# main.py
# import the necessary packages
from flask import Flask, render_template, Response
from camera import VideoCamera
import socket
import os

app = Flask(__name__)

# Configure HTML Web page ip and port settings
# Get wlan0 ip address of 'this' device
gw = os.popen("ip -4 route show default").read().split()
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.connect((gw[2], 0))
HTML_IPADDR = s.getsockname()[0]
HTML_PORT='5000'
s.close()


# localhost config
# Used to capturing C++ video stream over socket IPC
LOCAL_HOST = '127.0.0.1'
LOCAL_PORT = 4099

@app.route('/')
def index():
    # rendering webpage
    return render_template('index.html')

def gen(vServer):
	while True:
		#get camera frame
		frame = vServer.get_data()
		if (frame is not None):
			yield (b'--frame\r\n'
			       b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n\r\n')

@app.route('/video_feed')
def video_feed():
    video_feed = VideoCamera(LOCAL_HOST, LOCAL_PORT)
    return Response(gen(video_feed),
                    mimetype='multipart/x-mixed-replace; boundary=frame')


if __name__ == '__main__':
    # defining server ip address and port
    app.run(host=HTML_IPADDR,port=HTML_PORT, debug=False)
