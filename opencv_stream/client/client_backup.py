import cv2
import socket
import sys
import numpy as np

#HOST, PORT = "192.168.7.38", 4099
HOST, PORT = "127.0.0.1", 4098

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST,PORT))
#s.settimeout(5)

while(1):
    input_data = np.asarray(s.recv(307200))
    imgData = np.fromstring(input_data, dtype = 'uint8')
    if (imgData.size == 307200):
        print("Capture Frame")
        img = imgData.reshape([480, 640])
        cv2.imshow('frame', img)
        if cv2.waitKey(1) == ord('q'):
            break
    else:
        print("data mismatch")
# When everything done, release the capture
cv2.destroyAllWindows()
