# main.py
# import the necessary packages
from flask import Flask, render_template, Response
from camera import VideoCamera
from server import VidServer
app = Flask(__name__)
@app.route('/')
def index():
    # rendering webpage
    return render_template('index.html')

def gen(camera):
    while True:
        #get camera frame
        frame = camera.get_frame()
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n\r\n')


@app.route('/video_feed')
def video_feed():
    return Response(gen(VideoCamera()),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

#def gen(vServer):
#    while True:
#        #get camera frame
#        frame = vServer.get_data()
#        yield (b'--frame\r\n'
#               b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n\r\n')
#
#@app.route('/video_feed')
#def video_feed():
#    return Response(gen(VidServer('10.0.2.15', 4097)),
#                    mimetype='multipart/x-mixed-replace; boundary=frame')

if __name__ == '__main__':
    # defining server ip address and port
    app.run(host='0.0.0.0',port='5000', debug=True)
