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

RUN mv ./yolov2-master/model/ ./ && mv ./yolov2-master/utils ./ && \
	wget https://www.dropbox.com/s/xe8dyebmebomw42/yolo.pb?dl=0 -O ./model/yolo.pb

COPY Makefile *.jpg *.cpp *.build *.py \
	/coin_yolo/

CMD ["bash"]
