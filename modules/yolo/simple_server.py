#!/usr/bin/env python3

import os
import argparse
import socket            
 

def main():
    args = parse_args()

    with socket.socket() as s:
        # prevent "Already in use" errors if we kill then restart
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        s.bind(('', args.port))
        s.listen()
         
        c, addr = s.accept()
        print("Connected: ", addr)
        c.send("Hello, client".encode())
        print(c.recv(10))
        c.close()


def parse_args():
    description = """A very simple socket server, just an example,
    useful for dev work with the cam as a client.
    """

    parser = argparse.ArgumentParser(description=description)

    parser.add_argument("--port", "-p",
                        default=3451,
                        help="port to listen on, default: %(default)s")

    args = parser.parse_args()
    return args


if __name__ == "__main__":
    main()
