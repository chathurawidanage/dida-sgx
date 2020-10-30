FROM tozd/sgx:ubuntu-bionic
RUN apt update && apt install git -y && apt-get install -y \
                    make \
                    git \
                    curl \
                    vim \
                    wget \
                    gcc \
                    g++ 
RUN wget https://github.com/Kitware/CMake/releases/download/v3.18.4/cmake-3.18.4-Linux-x86_64.tar.gz
RUN tar -xvzf cmake-3.18.4-Linux-x86_64.tar.gz
# RUN wget https://download.open-mpi.org/release/open-mpi/v4.0/openmpi-4.0.1.tar.gz
# RUN tar -xvzf openmpi-4.0.1.tar.gz
# RUN cd openmpi-4.0.1 && mkdir build && ./configure --prefix=/openmpi-4.0.1/build && make && make install
# ENV MPI_HOME=/openmpi-4.0.1/build
RUN ls
RUN git clone https://github.com/chathurawidanage/dida-sgx.git
RUN mkdir dida-sgx/build
RUN cd dida-sgx && /cmake-3.18.4-Linux-x86_64/bin/cmake . && /cmake-3.18.4-Linux-x86_64/bin/cmake --build ./build
RUN ls dida-sgx/
RUN ls dida-sgx/
ENTRYPOINT dida-sgx/didasgx
CMD [""]