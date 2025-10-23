#Download base image.
#The ubuntu:20.04 tag points to the "20.04 LTS"
FROM ubuntu:20.04

#Set location to download ARM toolkit from.
# This will need to be changed over time or replaced with a static link to the latest release.
ENV ARMBINURL="https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2019q4/RC2.1/gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2?revision=6e63531f-8cb1-40b9-bbfc-8a57cdfc01b4&la=en&hash=F761343D43A0587E8AC0925B723C04DBFB848339"

#Create volume /havocbin for compiled firmware binaries
VOLUME /havocbin

#Copy build context (repository root) to /havocsrc
COPY ./ /havocsrc

# Fix line endings in HackRF files (Windows CRLF -> Unix LF)
RUN find /havocsrc/hackrf -type f -name "*.py" -o -name "*.sh" -o -name "*.mk" | xargs dos2unix 2>/dev/null || true
RUN find /havocsrc/hackrf -type f -name "Makefile*" | xargs dos2unix 2>/dev/null || true

#Fetch dependencies from APT
RUN apt-get update && \
        apt-get install -y tar wget dfu-util cmake python bzip2 lz4 curl python3 python3-yaml dos2unix && \
        apt-get -qy autoremove

#Install current pip from PyPa
RUN curl https://bootstrap.pypa.io/pip/3.4/get-pip.py -o get-pip.py && \
        python get-pip.py

#Fetch additional dependencies from Python 2.x pip
RUN pip install pyyaml
RUN rm -rf /usr/bin/python && \
        rm -rf /usr/bin/pip && \
        ln -s /usr/bin/python3 /usr/bin/python && \
        ln -s /usr/bin/pip3 /usr/bin/pip

ENV LANG C.UTF-8
ENV LC_ALL C.UTF-8

# Install ARM GCC toolchain from Debian/Ubuntu packages
RUN apt-get update && apt-get install -y gcc-arm-none-eabi

# Note: Using Debian package instead of downloading from ARM website

CMD cd /havocsrc && \
        mkdir -p build && cd build && \
        cmake .. && make firmware && \
        cp firmware/portapack-h1-havoc.bin /havocbin
