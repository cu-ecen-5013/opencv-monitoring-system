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
    bytes = 0
    data_list = []
    while(bytes < 307200):
        data = np.asarray(s.recv(307200))
        bytes += data.nbytes
        data_list.append(data.tostring())
    if(len(data_list) > 0):
        x = data_list[0]
        for i in range(1, len(data_list)):
            x += data_list[i]
        imgData = np.fromstring(x, dtype='uint8')
        if (imgData.size == 307200):
            img = imgData.reshape([480, 640])
            cv2.imshow('frame', img)
            if cv2.waitKey(1) == ord('q'):
                break
        else:
            print("data mismatch: %dbytes received" %imgData.size)
#        del imgData
#        del data_list

# When everything done, release the capture
cv2.destroyAllWindows()
