FROM tozd/sgx:ubuntu-bionic
RUN apt update && apt install git -y && apt-get install -y \
                    make \
                    git \
                    curl \
                    vim \
                    wget \
                    gcc \
                    g++ \
                    openmpi-bin libopenmpi-dev
RUN wget https://github.com/Kitware/CMake/releases/download/v3.18.4/cmake-3.18.4-Linux-x86_64.tar.gz
RUN tar -xvzf cmake-3.18.4-Linux-x86_64.tar.gz
ENV PATH="cmake-3.18.4-Linux-x86_64/bin:${PATH}"
RUN pwd
RUN git clone https://github.com/chathurawidanage/dida-sgx.git
RUN cd dida-sgx && /cmake-3.18.4-Linux-x86_64/bin/cmake .
