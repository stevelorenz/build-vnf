#!/usr/bin/python

import socket
import cv2
import sys
import struct
import time
import video_processor as vp


def img_to_bytes(img, fmt):
    success, encoded_img = cv2.imencode(fmt, img)
    msg = encoded_img.tobytes()
    return msg


def send_to_server(sock, info, dest_address):
    '''
    Function for sending decoded image to server
    @info image message with helper information [info payload]
    @dest_address(tuple) destination address: (ip, port)
    '''
    msg_len = len(info)
    sock.sendall(struct.pack('>L', msg_len) + info)
    # pre_value_indicate_maglen = struct.pack('!i', msg_len)
    # print("current msg size is: {}".format(msg_len))
    # _ = sock.sendto(pre_value_indicate_maglen, dest_address)

    # while totalsent < msg_len - 1:
    #     h_index = min(totalsent+max_len, msg_len)
    #     sent = sock.sendto(info[totalsent:h_index], dest_address)
    #     if sent == 0:
    #         print("error")
    #     totalsent = totalsent + sent
    # assert totalsent == msg_len
    # # fragment_size = 50000
    # # r = int(len(info) / fragment_size)
    # # rest = len(info) % fragment_size
    # # for i in range(r):
    # #     sock.sendto(info[fragment_size*i:fragment_size*(i+1)], dest_address)
    # # if rest != 0:
    # #     sock.sendto(info[r*fragment_size:(r*fragment_size+rest)], dest_address)
    # # # end signal for one image
    # # sock.sendto(b'', dest_address)


def receive_from_network(path):
    '''
    Function for receiving video packets(RTP, Mjpeg) and decoding
    path: ip:port or sdp file
    '''
    video_proc = vp.VideoProc(path, 432, 320)
    address = ('10.0.0.252', 8888)
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(address)
    print("connected to server")
    cnt = 0
    start = time.time()
    while(1):
        frame = video_proc.get_video_frame(path)
        # cv2.imshow("img", frame)
        # cv2.waitKey(1)
        msg = img_to_bytes(frame, '.png')
        send_to_server(s, msg, address)
        cnt += 1
        cur = time.time()
        print("frame {} sended".format(cnt), "fps: ", cnt / (cur - start))
    s.close()


if __name__ == "__main__":

    # img_path = "./yolo/pedes_images/01-20170320211734-05.jpg"
    # img = cv2.imread(img_path)
    # print("image shape:", img.shape)
    # msg = img_to_bytes(img, '.png')

    # address = ('10.0.0.252', 8888)
    # s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    # print(len(msg), sys.getsizeof(msg))
    # send_to_server(s, msg, address)

    # s.close()
    receive_from_network("./a.sdp")
