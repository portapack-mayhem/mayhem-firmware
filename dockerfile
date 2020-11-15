#Download base image.
#Pull Fixed ubuntu version to avoid future issues
FROM ubuntu:focal-20201008

#Set location to download ARM toolkit from.
# This will need to be changed over time or replaced with a static link to the latest release.
ENV ARMBINURL="https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2019q4/RC2.1/gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2?revision=6e63531f-8cb1-40b9-bbfc-8a57cdfc01b4&la=en&hash=F761343D43A0587E8AC0925B723C04DBFB848339"

#Set Timezone to avoid interaction during tzdata package installation
ENV TZ=Europe/Paris
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

#Create volume /havocbin for compiled firmware binaries
VOLUME /havocbin

#Copy build context (repository root) to /havocsrc
COPY ./ /havocsrc

#Fetch dependencies from APT
RUN apt update && \
        apt install -qy tar wget dfu-util cmake python3 python3-pip git bzip2 curl && \
        apt -qy autoremove


#Fetch additional dependencies from Python pip
RUN pip3 install pyyaml
RUN ln -sf /usr/bin/python3 /usr/bin/python && \
    ln -sf /usr/bin/pip3 /usr/bin/pip

ENV LANG C.UTF-8
ENV LC_ALL C.UTF-8

#Grab the GNU ARM toolchain from arm.com
#Then extract contents to /opt/build/armbin/
RUN mkdir /opt/build && cd /opt/build && \
        wget -O gcc-arm-none-eabi $ARMBINURL && \
        mkdir armbin && \
        tar --strip=1 -xjvf gcc-arm-none-eabi -C armbin

#Set environment variable so compiler knows where the toolchain lives
ENV PATH=$PATH:/opt/build/armbin/bin

CMD cd /havocsrc && git submodule update --init --recursive && \
    mkdir -p /havocsrc/build && cd /havocsrc/build && \
    cmake .. && make firmware && \
    cp /havocsrc/build/firmware/portapack-h1_h2-mayhem.bin /havocbin
