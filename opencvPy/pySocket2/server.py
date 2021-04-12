import pickle
import socket
import struct
import cv2


class VidServer():
	def __init__(self, host_ip, port):
		HOST = host_ip
		PORT = port

		self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		print('Socket created')

		self.s.bind((HOST, PORT))
		print('Socket bind complete')
		self.s.listen(10)
		print('Socket now listening')

		self.conn, self.addr = self.s.accept()
		print("Connection accepted")

	def get_data(self):
		data = b'' ### CHANGED
		payload_size = struct.calcsize("L") ### CHANGED
		while True:

		    # Retrieve message size
		    while len(data) < payload_size:
		        data += self.conn.recv(4096)
		    packed_msg_size = data[:payload_size]
		    data = data[payload_size:]
		    msg_size = struct.unpack("L", packed_msg_size)[0] ### CHANGED

		    # Retrieve all data based on message size
		    while len(data) < msg_size:
		        data += self.conn.recv(4096)
		    frame_data = data[:msg_size]
		    data = data[msg_size:]

		    # Extract frame
		    frame = pickle.loads(frame_data)
		    print(frame)
		    print(frame.shape)
		    # Display
		    cv2.imshow('frame', frame)
		    cv2.waitKey(1)

ser = VidServer('10.0.2.15', 4097)
ser.get_data()
