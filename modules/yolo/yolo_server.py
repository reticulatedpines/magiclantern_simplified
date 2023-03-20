#!/usr/bin/env python3

import os
import argparse
import socket            
import binascii
import struct
import time

import numpy as np
import cv2

def main():
    args = parse_args()

    with socket.socket() as s:
        # prevent "Already in use" errors if we kill then restart
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        s.bind(('', args.port))
         
        s.listen()
         
        client, addr = s.accept()
        print("Connected: ", addr)

        i = 0
        max_recv = 1460
        msg = client.recv(max_recv)
        len_left = 0
        is_frame_start = True
        while True:
            # The cam sends one frame of LV at a time, using data format:
            # uint8_t type, currently unused.
            # uint32_t size of following data.
            # array of uint8_t data, comprising UYVY data from liveview.
            if is_frame_start:
                start_time = time.time_ns() // 1000000
                t = msg[0:1]
                total_len = struct.unpack("<L", msg[1:5])[0]
                print("Length: %d" % total_len)
                data = msg[5:]
                len_left = total_len - len(data)
                is_frame_start = False
            else:
                # continuing data, no TL prefix
                msg_len = len(msg)
                if len_left < max_recv:
                    print("low left, len(msg): %d" % msg_len)
                if len_left < msg_len:
                    print("Overlap? " + binascii.hexlify(msg[0:32]).decode())
                    data += msg[:len_left]
                    msg = msg[len_left:]
                    len_left = 0
                    show_yuv(data)
                    is_frame_start = True
                    continue # skip the recv so we process the remaining data in msg
                else:
                    len_left -= msg_len
                    data += msg

            if len_left < 0:
                print("len_left went negative!")
                break

            if len_left == 0:
                # we got a complete frame
                end_time = time.time_ns() // 1000000
                print("Time for frame: %d ms" % (end_time - start_time))
                show_yuv(data)
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


def show_yuv(data):
    yuv = np.frombuffer(data, dtype=np.uint8)
    yuv = yuv.reshape(480, 736, 2)
    bgr = cv2.cvtColor(yuv, cv2.COLOR_YUV2BGR_UYVY)
    cv2.imshow("frame", bgr)
    cv2.waitKey(0)
    cv2.destroyAllWindows()
    cv2.waitKey(1)


def parse_args():
    description = """Server for receiving and displaying LV data from cam via network
    """

    parser = argparse.ArgumentParser(description=description)

    parser.add_argument("--port", "-p",
                        default=3451,
                        help="port to listen on, default: %(default)s")

    args = parser.parse_args()
    return args


if __name__ == "__main__":
    main()
