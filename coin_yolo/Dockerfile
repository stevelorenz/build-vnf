# This image is TOO bloated...
# I got this image from a student who had already graduated...
# Nobody can/wants to refine this setup...

FROM ffpp:0.6.0

RUN cd /ffpp && make install > /dev/null

# Install YOLOv2 tensorflow dependencies
RUN apt-get update && apt-get install -y \
	libfontconfig1 \
	libgl1-mesa-glx \
	libsm6 \
	libxext6 \
	libxrender1 \
	software-properties-common \
	zip

WORKDIR /coin_yolo
RUN wget https://github.com/stevelorenz/yolov2/archive/refs/heads/master.zip && \
	unzip ./master.zip -d ./ && \
	rm ./master.zip
COPY ./environment.yml ./
RUN wget --quiet https://repo.continuum.io/archive/Anaconda3-5.0.1-Linux-x86_64.sh -O ~/anaconda.sh && \
	/bin/bash ~/anaconda.sh -b -p /opt/conda && \
	rm ~/anaconda.sh && \
	/opt/conda/bin/conda env create -f /coin_yolo/environment.yml && \
	echo "source activate tf_mkl" > ~/.bashrc
ENV PATH /opt/conda/envs/tf_mkl/bin:$PATH
ENV CONDA_DEFAULT_ENV tf_mkl
# Fix the broken numpy version
RUN conda install -y numpy=1.15.0
# Avoid encoding problem for Python3
ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

RUN mv ./yolov2-master/model/ ./ && mv ./yolov2-master/utils ./ && \
	wget https://www.dropbox.com/s/xe8dyebmebomw42/yolo.pb?dl=0 -O ./model/yolo.pb

RUN wget http://images.cocodataset.org/annotations/annotations_trainval2017.zip && \
    unzip ./annotations_trainval2017.zip -d ./cocoapi/ && \
    rm ./annotations_trainval2017.zip ./cocoapi/annotations/instances_train2017.json ./cocoapi/annotations/person_keypoints_train2017.json

RUN apt-get update && apt-get install -y \
	sockperf python3-psutil

# Try to reduce image size...
RUN rm -rf /var/lib/apt/lists/* && \
    conda clean -y --all && \
    rm -rf /opt/conda/pkgs/*

# Build coin_yolo related programs
COPY Makefile *.jpg *.cpp *.build *.py *.sh \
	/coin_yolo/
RUN make release

CMD ["bash"]
