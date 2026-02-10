###########################################
# Base image with ROS2
###########################################
FROM ros:jazzy AS base

ENV DEBIAN_FRONTEND=noninteractive
ARG ROS_DISTRO=jazzy
ENV ROS_DISTRO=${ROS_DISTRO}

# Install common programs
RUN apt-get update && apt-get install -y --no-install-recommends \
    curl \
    gnupg2 \
    lsb-release \
    sudo \
    software-properties-common \
    wget \
    python3-rosdep \
    && rm -rf /var/lib/apt/lists/*

RUN rosdep init || echo "rosdep already initialized"

ENV DEBIAN_FRONTEND=

# Setup entrypoint
COPY ./ros_entrypoint.sh /
ENTRYPOINT ["/ros_entrypoint.sh"]
CMD ["bash"]

###########################################
# Dev image with development tools
###########################################
FROM base AS dev

ENV DEBIAN_FRONTEND=noninteractive

# Install development tools and ROS2 dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    bash-completion \
    build-essential \
    cmake \
    gdb \
    git \
    openssh-client \
    python3-pip \
    python3-argcomplete \
    valgrind \
    clang-format \
    clang-tidy \
    nvim \
    nano \
    htop \
    tree \
    ros-${ROS_DISTRO}-rmw-cyclonedds-cpp \
    ros-${ROS_DISTRO}-rosidl-generator-dds-idl \
    ros-dev-tools \
    && rm -rf /var/lib/apt/lists/*

# Install LLVM 18 (for C++ compilation)
RUN wget --progress=dot:giga https://apt.llvm.org/llvm.sh && chmod +x llvm.sh && ./llvm.sh 18 all
RUN update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-18 100 \
    && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 100 \
    && update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 100 \
    && update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-18 100 \
    && update-alternatives --install /usr/bin/run-clang-tidy run-clang-tidy /usr/bin/run-clang-tidy-18 100

ENV CC=/usr/bin/clang-18
ENV CXX=/usr/bin/clang++-18
ENV DEBIAN_FRONTEND=

# Setup workspace directory
WORKDIR /workspace

# Setup bash environment with autocompletion
RUN echo "if [ -f /opt/ros/${ROS_DISTRO}/setup.bash ]; then source /opt/ros/${ROS_DISTRO}/setup.bash; fi" >> ~/.bashrc \
    && echo "if [ -f /workspace/install/setup.bash ]; then source /workspace/install/setup.bash; fi" >> ~/.bashrc \
    && echo "if [ -f /usr/share/colcon_argcomplete/hook/colcon-argcomplete.bash ]; then source /usr/share/colcon_argcomplete/hook/colcon-argcomplete.bash; fi" >> ~/.bashrc \
    && echo "alias cb='colcon build --symlink-install'" >> ~/.bashrc \
    && echo "alias cbs='colcon build --symlink-install --packages-select'" >> ~/.bashrc \
    && echo "alias ct='colcon test'" >> ~/.bashrc \
    && echo "alias ctr='colcon test-result --verbose'" >> ~/.bashrc \
    && echo "alias sw='source /workspace/install/setup.bash'" >> ~/.bashrc

###########################################
# Go2 robot image with packages
###########################################
FROM dev AS go2

ENV DEBIAN_FRONTEND=noninteractive

# Copy Go2 Robot-specific ROS2 packages
COPY ./packages /workspace/src/

# Build the ROS2 workspace
WORKDIR /workspace

# Update package lists and install dependencies
RUN apt-get update \
    && . /opt/ros/${ROS_DISTRO}/setup.sh \
    && rosdep install --from-paths src --ignore-src -r -y \
    && rm -rf /var/lib/apt/lists/*

# Build workspace with debug symbols for development
RUN . /opt/ros/${ROS_DISTRO}/setup.sh \
    && colcon build --cmake-args -DCMAKE_BUILD_TYPE=RelWithDebInfo

ENV DEBIAN_FRONTEND=
