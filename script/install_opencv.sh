#! /bin/bash
#
# install_opencv.sh
#
# About: Install OpenCV on Ubuntu 16.04
#

OPENCV_VERSION="3.4.1"
PIP_VERSION="9.0.3"

sudo apt-get update

echo "Install dependencies..."
sudo apt-get install -y build-essential checkinstall cmake pkg-config yasm
sudo apt-get install -y git gfortran
sudo apt-get install -y libjpeg8-dev libjasper-dev libpng12-dev

sudo apt-get install -y libtiff5-dev

sudo apt-get install -y libavcodec-dev libavformat-dev libswscale-dev libdc1394-22-dev
sudo apt-get install -y libxine2-dev libv4l-dev
sudo apt-get install -y libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev
sudo apt-get install -y qt5-default libgtk2.0-dev libtbb-dev
sudo apt-get install -y libatlas-base-dev
sudo apt-get install -y libfaac-dev libmp3lame-dev libtheora-dev
sudo apt-get install -y libvorbis-dev libxvidcore-dev
sudo apt-get install -y libopencore-amrnb-dev libopencore-amrwb-dev
sudo apt-get install -y x264 v4l-utils

# Optional dependencies
sudo apt-get install -y libprotobuf-dev protobuf-compiler
sudo apt-get install -y libgoogle-glog-dev libgflags-dev
sudo apt-get install -y libgphoto2-dev libeigen3-dev libhdf5-dev doxygen

echo "Install Python libs..."
sudo apt-get install -y python-dev python-pip python3-dev python3-pip

sudo -H pip3 install --upgrade pip=="$PIP_VERSION"

# Use python3
sudo -H pip3 install numpy scipy matplotlib scikit-image scikit-learn ipython imutils

# Download and install OpenCV
cd ~ || exit
git clone https://github.com/opencv/opencv.git
cd opencv || exit
git checkout -b "$OPENCV_VERSION"
cd ~ || exit

git clone https://github.com/opencv/opencv_contrib.git
cd opencv_contrib
git checkout -b "$OPENCV_VERSION"
cd ~ || exit

cd opencv || exit
mkdir build
cd build || exit

cmake -D CMAKE_BUILD_TYPE=RELEASE \
    -D CMAKE_INSTALL_PREFIX=/usr/local \
    -D INSTALL_C_EXAMPLES=ON \
    -D INSTALL_PYTHON_EXAMPLES=ON \
    -D OPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules \
    -D BUILD_EXAMPLES=ON ..

echo "Start compiling OpenCV... This take time..."
make -j 2
sudo make install

sudo sh -c 'echo "/usr/local/lib" >> /etc/ld.so.conf.d/opencv.conf'
sudo ldconfig

sudo apt install -y ipython3

echo "Congrats, OpenCV installation finished!"
echo "Try to run some samples in the ~/opencv/samples/python to check the installation."
echo "[WARN] Only Python3 packages are installed, run samples with python3."
