###########################################
# Septentrio image with
###########################################
FROM ubuntu:latest AS rxtools
ENV WORKDIR=workspace
ENV DEBIAN_FRONTEND=noninteractive

# Dependencies for rxTools see https://github.com/septentrio-gnss/septentrio_gnss_driver/issues/167
RUN apt update

# Qt6   
RUN apt install -y \
    qt6-base-dev \
    qt6-base-dev-tools \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    libqt6core6 \
    libqt6gui6 \
    libqt6widgets6 \
    qt6-wayland \
    libqt6waylandclient6 \
    libqt6waylandcompositor6

# XCB
RUN apt install -y \
    libxcb1 \
    libxcb-cursor0 \
    libxcb-util1 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-xinerama0 \
    libxcb-xinput0 \
    libxcb-xkb1 \
    libxkbcommon-x11-0

# X11
RUN apt install -y \
    libx11-6 \
    libxext6 \
    libxrender1 \
    libxfixes3 \
    libxi6 \
    libxrandr2 \
    libxcomposite1 \
    libxcursor1 \
    libxdamage1 \
    libxss1 \
    libxtst6

# wayland
RUN apt install -y \
    libwayland-client0 \
    libwayland-server0 \
    libwayland-egl1 \
    libwayland-cursor0

# OpenGL
RUN apt install -y \
    libgl1 \
    libopengl0 \
    libglx-mesa0 \
    libgl1-mesa-dri

# General dependencies
RUN apt install -y \
    libglib2.0-0 \
    libdbus-1-3 \
    libfontconfig1 \
    libfreetype6

RUN export QT_QPA_PLATFORM=xcb
RUN export QT_DEBUG_PLUGINS=1

RUN apt install -y usbutils

RUN apt-get update && apt-get install -y \
    git \
    python3 \
    pip \
    python3-venv

WORKDIR /$WORKDIR
COPY ./rxtools_26_0_0_installer_x64_0.bin /$WORKDIR

RUN git clone https://github.com/BerendPijnenburg/Septentrio-PyDataLink.git
WORKDIR /$WORKDIR/Septentrio-PyDataLink
RUN python3 build.py
RUN mv PyDataLink /$WORKDIR
RUN rm -rf /$WORKDIR/Septentrio-PyDataLink
WORKDIR /$WORKDIR

# add rxTools if x64 image
RUN case "$(uname -m)" in \
        x86_64) echo "Running rxtools_x64 setup" && \
        mkdir /root/Desktop && \
        chmod +x rxtools_26_0_0_installer_x64_0.bin && \
        ./rxtools_26_0_0_installer_x64_0.bin in --am --al -c && \
        echo "export PATH=/opt/Septentrio/RxTools/bin:$PATH" >> /root/.bashrc && \
        echo "echo 'available Septentrio tools: \n\
        runAps3gtools\n\
        runDatalink\n\
        runRxassistant\n\
        runRxcontrol\n\
        runRxdownload\n\
        runRxlauncher\n\
        runRxleverarm\n\
        runRxlogger\n\
        runRxplanner\n\
        runRxupgrade\n\
        runSbfanalyzer\n\
        runSbfconverter'" >> /root/.bashrc; ;; \
        *) echo "Skipping rxtools_x64 step on non x64 devices"; ;; \
    esac
RUN echo "echo 'For ARM NTRIP forwarding use ./PyDataLink \n\
        NOTE: This software is likely not the most stable, the BNC ntrip client is recommended.'" >> /root/.bashrc
RUN rm rxtools_26_0_0_installer_x64_0.bin

ENV DEBIAN_FRONTEND=
