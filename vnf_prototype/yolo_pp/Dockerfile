#
# About: Dockerfile to build YOLO (v2) preprocessing application
#
# WARN: This image is huge(around 9GB). It is currently only for functionality
# and performance testing, the image size is not considered.
#

FROM fastio_user:latest

# Install YOLOv2 test tool from zrbzrb1106

RUN apt-get update && apt-get install -y \
        net-tools \
        iputils-ping \
        iproute2 \
        software-properties-common \
        sudo \
        git \
        wget \
        python3-pip \
        zip \
        libsm6 \
        libxext6 \
        libfontconfig1 \
        libxrender1 \
        libgl1-mesa-glx

WORKDIR /app
RUN git clone https://github.com/zrbzrb1106/yolov2.git

RUN wget --quiet https://repo.continuum.io/archive/Anaconda3-5.0.1-Linux-x86_64.sh -O ~/anaconda.sh && \
        /bin/bash ~/anaconda.sh -b -p /opt/conda && \
        rm ~/anaconda.sh

RUN /opt/conda/bin/conda env create -f /app/yolov2/dockerfiles/environment.yml
RUN echo "source activate tf_mkl" > ~/.bashrc
ENV PATH /opt/conda/envs/tf_mkl/bin:$PATH
ENV CONDA_DEFAULT_ENV tf_mkl

RUN conda install numpy=1.15.0

RUN useradd -m docker && echo "docker:docker" | chpasswd && adduser docker sudo

WORKDIR /app/yolov2

RUN wget --quiet https://cloudstore.zih.tu-dresden.de/index.php/s/MQxalL8fpKkGfW0/download -O ./lola_yolo_dataset.zip && \
        unzip ./lola_yolo_dataset.zip && \
        rm ./lola_yolo_dataset.zip

COPY ./preprocess.py /app/yolov2/preprocess.py
COPY ./test_preprocess.py /app/yolov2/test_preprocess.py

CMD ["bash"]
