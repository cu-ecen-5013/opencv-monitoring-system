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

@app.route('/')
def index():
    # rendering webpage
    return render_template('index.html')

if __name__ == '__main__':
    # defining server ip address and port
    app.run(host=HTML_IPADDR, port=HTML_PORT, debug=True)
