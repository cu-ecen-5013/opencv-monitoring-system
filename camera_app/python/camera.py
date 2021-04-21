import cv2
import socket
import sys
import numpy as np
import errno
from socket import error as socket_error

ds_factor=0.6

class VideoCamera(object):
	def __init__(self, HOST, PORT):
		#Setup socket connection
		self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		try:
			self.s.connect((HOST,PORT))
			print("Connection accepted")
		except socket_error as serr:
			if serr.errno != errno.ECONNREFUSED:
				raise serr
			print(serr.errno)
	def __del__(self):
		#releasing camera
		self.s.close()
		print("Connection closed")

	def get_data(self):
		bytes = 0
		data_list = []
		while(bytes < 307200):
			data = np.asarray(self.s.recv(307200))
			bytes += data.nbytes
			data_list.append(data.tostring())
		if(len(data_list) > 0):
			x = data_list[0]
			for i in range(1, len(data_list)):
				x += data_list[i]
			imgData = np.fromstring(x, dtype='uint8')
			if (imgData.size == 307200):
				frame = imgData.reshape([480, 640])
				ret, jpeg = cv2.imencode('.jpg', frame)
#				cv2.imshow('Python OPENCV Video Client', frame)
#				cv2.waitKey(1)
				return jpeg.tobytes()
#				if cv2.waitKey(1) == ord('q'):
#					break
			else:
				print("data mismatch: %dbytes received" %imgData.size)


