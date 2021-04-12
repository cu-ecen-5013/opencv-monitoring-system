import socket, cv2, pickle,struct
import numpy as np

HOST = '127.0.0.1'
PORT = 5000
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

socket_address = (HOST, PORT)
server.bind(socket_address)
server.listen(5)
print("Server Listening on:",socket_address)

video = cv2.VideoCapture(0)


while True:
    client_socket,addr = server.accept()
#    print('GOT CONNECTION FROM:',addr)
    ret, frame = video.read()
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    cv2.imshow('TRANSMITTING VIDEO', gray)
#    if client_socket:
#        print("Sending video")
#        if img:
#            a = pickle.dumps(frame)
#            message = struct.pack("Q",len(a))+a
#            client_socket.sendall(message)
    if (cv2.waitKey(1) & 0xFF == ord('q')):
#        client_socket.close()
        break

# When everything done, release the capture
video.release()
cv2.destroyAllWindows()
