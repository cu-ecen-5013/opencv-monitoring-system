# main.py
# import the necessary packages
from flask import Flask, render_template, Response
from camera import VideoCamera
app = Flask(__name__)

# HTML Web page config
#HTML_HOST='192.168.7.38'
HTML_HOST='127.0.0.1'
HTML_PORT= '5000'

# localhost config
LOCAL_HOST = '127.0.0.1'
LOCAL_PORT = 4098

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
    return Response(gen(VideoCamera(LOCAL_HOST, LOCAL_PORT)),
                    mimetype='multipart/x-mixed-replace; boundary=frame')



if __name__ == '__main__':
    # defining server ip address and port
    app.run(host=HTML_HOST,port=HTML_PORT, debug=True)
