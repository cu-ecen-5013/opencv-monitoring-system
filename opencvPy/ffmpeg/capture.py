'''capture.py'''
import sys
import cv2
cap = cv2.VideoCapture(0)                    # 0 is for /dev/video0
while True :
    if not cap.read() : break
    ret, frame = cap.read()
    sys.stdout.write( str(frame.tostring()) )
