#!/usr/bin/env python3

import os
import argparse
import socket            
import binascii
import struct
import time
from collections import namedtuple

import numpy as np
import cv2 as cv

# simple class for detection boxes, with category name
Box = namedtuple("Box", "name x y w h")

darknet_dir = ""
net = None

def main():
    args = parse_args()

    # the location where darknet code lives
    global darknet_dir
    darknet_dir = args.darknet_dir

    init_YOLO()

    with socket.socket() as s:
        # prevent "Already in use" errors if we kill then restart
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        s.bind(("", args.port))
        s.listen()
         
        client, addr = s.accept()
        print("Cam connected to server, from: ", addr)

        i = 0
        # This relates to MTU on your network, too large and packets
        # will get split in an inefficient way.
        max_recv = 1460
        msg = client.recv(max_recv)
        len_left = 0
        is_frame_start = True
        while True:
            # The cam sends one frame of LV at a time, using data format:
            # uint8_t type, currently unused.
            # uint32_t size of following data.
            # array of uint8_t data, comprising UYVY data from liveview.
            #
            # Every frame we receive, we send back detected objects
            # (code will deadlock if no response is sent).
            if is_frame_start:
                start_time = time.time_ns() // 1000000
                t = msg[0:1]
                total_len = struct.unpack("<L", msg[1:5])[0]
                print("\nFrame size: %d" % total_len)
                data = msg[5:]
                len_left = total_len - len(data)
                is_frame_start = False
            else:
                # continuing data, no TL prefix
                msg_len = len(msg)
                if len_left < max_recv:
                    pass
                    #print("low left, len(msg): %d" % msg_len)
                len_left -= msg_len
                data += msg

            if len_left < 0:
                print("len_left went negative!")
                break

            if len_left == 0:
                # we got a complete frame
                end_time = time.time_ns() // 1000000
                print("Time for frame: %d ms" % (end_time - start_time))
                
                detections = yolo_detect(data)
                response = create_response(detections)
                #print(binascii.hexlify(response))
                client.send(response)

                is_frame_start = True
                read_len = max_recv
            elif len_left > max_recv:
                read_len = max_recv
            elif len_left > 0:
                read_len = len_left

            #print("Reading %d bytes, left %d" % (read_len, len_left))
            msg = client.recv(read_len)

        client.close()
    client.close()


def create_response(detections):
    """
    Given a list of Box namedtuples, format the data appropriately
    for sending to client on cam.

    Format is the following fields in order:
    1 byte for type
    1 byte for count of detections
    4 bytes for size of following data
    Payload data, an array of the following:
        Null terminated string
        x, y, w, h of the detection box, each as a uint16_t

    """
    # we encode number of detections in a byte,
    # truncate if too many
    if len(detections) > 254:
        detections = detections[:254]

    r = b""
    for d in detections:
        old_r = r

        r += d.name.encode()
        r += b"\0"
        r += struct.pack("<H", d.x)
        r += struct.pack("<H", d.y)
        r += struct.pack("<H", d.w)
        r += struct.pack("<H", d.h)

        # our response packet is limited by SERVER_RESP_MAX
        # in yolo.c, throw away detections if we can't fit them all.
        data_len = len(r)
        if data_len > 256 - 7:
            print("r too long, too many detections?")
            print(detections)
            r = old_r
            break

    data_len = len(r)
    r = struct.pack("<L", data_len) + r
    r = struct.pack("<B", len(detections)) + r
    r = b"\x01" + r # type field

    return r


def yolo_detect(data):
    """
    Use OpenCV and YOLO to detect objects in the given YUV data.
    """
    dets = get_detections(data)
    for d in dets:
        print(d)
    return dets


def init_YOLO():
    global net
    net = cv.dnn.readNetFromDarknet(os.path.join(darknet_dir, "cfg", "yolov3.cfg"),
                                    os.path.join(darknet_dir, "yolov3.weights"))
    net.setPreferableBackend(cv.dnn.DNN_BACKEND_OPENCV)


def get_detections(data):
    """
    Given some yuv image data, YOLO detect objects,
    return a list of Box namedtuples for the objects.
    """
    yuv = np.frombuffer(data, dtype=np.uint8)
    yuv = yuv.reshape(480, 736, 2)
    bgr = cv.cvtColor(yuv, cv.COLOR_YUV2BGR_UYVY)

    classes = open(os.path.join(darknet_dir, "cfg", "coco.names")).read().strip().split("\n")

    # determine the output layer
    ln = net.getLayerNames()
    ln = [ln[i - 1] for i in net.getUnconnectedOutLayers()]

    # YOLO detect
    blob = cv.dnn.blobFromImage(bgr, 1/255.0, (416, 416), swapRB=True, crop=False)
    net.setInput(blob)
    start_time = time.time_ns() // 1000000
    outputs = net.forward(ln)
    end_time = time.time_ns() // 1000000
    print("Detect time: %d ms" % (end_time - start_time))

    # get all detection boxes
    boxes = []
    confidences = []
    classIDs = []
    h, w = bgr.shape[:2]
    for output in outputs:
        for detection in output:
            scores = detection[5:]
            classID = np.argmax(scores)
            confidence = scores[classID]
            if confidence > 0.75:
                box = detection[:4] * np.array([w, h, w, h])
                (centerX, centerY, width, height) = box.astype("int")
                x = int(centerX - (width / 2))
                y = int(centerY - (height / 2))
                box = [x, y, int(width), int(height)]
                boxes.append(box)
                confidences.append(float(confidence))
                classIDs.append(classID)

    # filter for high confidence detections,
    # created named boxes
    high_conf_boxes = []
    indices = cv.dnn.NMSBoxes(boxes, confidences, 0.5, 0.4)
    if len(indices) > 0:
        for i in indices.flatten():
            (x, y) = (boxes[i][0], boxes[i][1])
            (w, h) = (boxes[i][2], boxes[i][3])
            # Sometimes the co-ords are negative!
            # Makes no sense to me.
            if x < 0:
                x = 2
            if y < 0:
                y = 2
            if w < 0:
                w = 2
            if h < 0:
                h = 2
            high_conf_boxes.append(Box(classes[classIDs[i]], x, y, w, h))

    return high_conf_boxes


def show_yuv(data):
    yuv = np.frombuffer(data, dtype=np.uint8)
    yuv = yuv.reshape(480, 736, 2)
    bgr = cv2.cvtColor(yuv, cv2.COLOR_YUV2BGR_UYVY)
    cv2.imshow("frame", bgr)
    cv2.waitKey(0)
    cv2.destroyAllWindows()
    cv2.waitKey(1)


def parse_args():
    description = """Server for receiving image data from cam via network,
    running YOLOv3 detection and sending back any detections.
    """

    parser = argparse.ArgumentParser(description=description)

    parser.add_argument("--port", "-p",
                        default=3451,
                        help="port to listen on, default: %(default)s")

    parser.add_argument("darknet_dir",
                        help="path to YOLO / darknet dir, which "
                             "contains cfg dir and YOLO weight file")

    args = parser.parse_args()
    return args


if __name__ == "__main__":
    main()
