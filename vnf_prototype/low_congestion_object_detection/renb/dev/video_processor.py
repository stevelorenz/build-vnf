#!/usr/bin/python

import subprocess
import datetime
import numpy as np
import cv2
import fcntl
import os


class VideoProc:
    '''
    fileloc: ip:port or sdp file
    '''

    def __init__(self, fileloc, width, height):
        self.get_stream_command = ['ffmpeg',
                                   '-loglevel', 'fatal',
                                   '-protocol_whitelist', 'file,udp,rtp',
                                   '-i', fileloc,
                                   '-vf', 'showinfo',
                                   '-f', 'image2pipe',
                                   '-pix_fmt', 'rgb24',
                                   '-an', '-sn',
                                   '-vcodec', 'rawvideo', '-']
        self.get_info_command = ['ffprobe',
                                 '-v', 'fatal',
                                 '-show_entries', 'stream=width,height,r_frame_rate,duration',
                                 '-protocol_whitelist', 'file,udp,rtp',
                                 fileloc]
        self.get_stream_sp = subprocess.Popen(
            self.get_stream_command, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
        self.width = width
        self.height = height
        self.nbytes = 3 * width * height

    def get_video_info(self, fileloc):
        '''
        fileloc is a.sdp due to rtp protocol
        '''
        command = self.get_info_command
        ffmpeg = subprocess.Popen(
            command, stderr=subprocess.PIPE, stdout=subprocess.PIPE, universal_newlines=True)
        out, err = ffmpeg.communicate()
        if(err):
            print(err)
        out = out.split('\n')
        return {'file': fileloc,
                'type': out[0].strip('[,]'),
                'width': int(out[1].split('=')[1]),
                'height': int(out[2].split('=')[1]),
                'fps': float(out[3].split('=')[1].split('/')[0])/float(out[3].split('=')[1].split('/')[1]),
                'duration': out[4].split('=')[1]}

    def get_video_frame(self, fileloc):
        result = ""
        try:
            s = self.get_stream_sp.stdout.read(self.nbytes)
            assert len(s) == self.nbytes
            result = np.fromstring(s, dtype='uint8')
            result = result.reshape((self.height, self.width, 3))
            result = result[..., ::-1]
            # cv2.imshow("result", result)
            # cv2.waitKey(1)
            self.get_stream_sp.stdout.flush()
        except Exception as e:
            print(e)
        return result
