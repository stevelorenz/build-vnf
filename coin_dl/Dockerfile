# The image size is still big (about 6GB) with Miniconda.
# Unused dependencies should be removed later after finishing paper draft.

FROM ffpp:0.8.0

RUN apt-get update && apt-get install -y --no-install-recommends \
	ethtool \
	libfontconfig1 \
	libgl1-mesa-glx \
	libsm6 \
	libxext6 \
	libxrender1 \
	software-properties-common \
	unzip \
	zip

WORKDIR /coin_dl
RUN wget https://github.com/stevelorenz/yolov2/archive/refs/heads/master.zip && \
	unzip ./master.zip -d ./ && \
	rm ./master.zip

COPY ./environment.yml ./
ENV PATH="/opt/conda/bin:${PATH}"
RUN wget --quiet https://repo.continuum.io/miniconda/Miniconda3-latest-Linux-x86_64.sh -O ~/miniconda.sh && \
	/bin/bash ~/miniconda.sh -b -p /opt/conda && \
	rm ~/miniconda.sh && \
	/opt/conda/bin/conda env create -f /coin_dl/environment.yml && \
	echo "source activate tf_mkl" > ~/.bashrc

ENV PATH /opt/conda/envs/tf_mkl/bin:$PATH
ENV CONDA_DEFAULT_ENV tf_mkl
# Avoid encoding problem for Python3
ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

# Download pre-trained YOLOv2 model from my public dropbox folder
RUN mv ./yolov2-master/model/ ./ && mv ./yolov2-master/utils ./ && \
	wget https://www.dropbox.com/s/xe8dyebmebomw42/yolo.pb?dl=0 -O ./model/yolo.pb

RUN wget http://images.cocodataset.org/annotations/annotations_trainval2017.zip && \
	unzip ./annotations_trainval2017.zip -d ./cocoapi/ && \
	rm ./annotations_trainval2017.zip ./cocoapi/annotations/instances_train2017.json ./cocoapi/annotations/person_keypoints_train2017.json

RUN apt-get update && apt-get install -y \
	sockperf iperf3

# Try to reduce image size...
RUN rm -rf /var/lib/apt/lists/* && \
	conda clean -y --all && \
	rm -rf /opt/conda/pkgs/*

# Build coin_dl related programs
COPY Makefile pedestrain.jpg coin_dl.cpp meson.build \
	*.py \
	*.sh \
	/coin_dl/
RUN make clean && make release

CMD ["bash"]
