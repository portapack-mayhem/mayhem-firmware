#Download base image.
#The ubuntu:latest tag points to the "latest LTS"
FROM ubuntu:latest
VOLUME /myvol

RUN apt-get update && \
    apt-get install -y software-properties-common && \
    apt-get install -y apt-transport-https

# 
RUN add-apt-repository ppa:team-gcc-arm-embedded/ppa && apt-get update && apt-get install -y gcc-arm-embedded dfu-util cmake git python python-pip && apt-get -qy autoremove
RUN pip install pyyaml

CMD git clone https://github.com/furrtek/portapack-havoc.git portapack-havoc && cd portapack-havoc && mkdir build && cd build && cmake .. && make firmware && cp /portapack-havoc/firmware/portapack-h1-havoc.bin /myvol
