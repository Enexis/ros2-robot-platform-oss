###########################################
# Base image with ROS2
###########################################
FROM ros:jazzy AS base

ENV DEBIAN_FRONTEND=noninteractive
ARG ROS_DISTRO=jazzy
ENV ROS_DISTRO=${ROS_DISTRO}
ENV WORKDIR=workspace
ARG COLCON_BUILD_SEQUENTIAL

# Install common programs
RUN apt-get update && apt-get install -y --no-install-recommends \
    curl \
    iputils-ping \
    gnupg2 \
    lsb-release \
    sudo \
    software-properties-common \
    wget \
    python3-rosdep \
    ros-$ROS_DISTRO-rmw-cyclonedds-cpp \
    ros-$ROS_DISTRO-rosidl-generator-dds-idl 

RUN rosdep init || echo "rosdep already initialized"
RUN rosdep update

# Setup bash environment
RUN echo "\
if [ -f \"/opt/ros/${ROS_DISTRO}/setup.bash\" ]; then\n\
    source \"/opt/ros/${ROS_DISTRO}/setup.bash\"\n\
fi\n\
if [ -f \"/${WORKDIR}/install/setup.bash\" ]; then\n\
    source \"/${WORKDIR}/install/setup.bash\"\n\
fi" >> /root/.bashrc

ENV DEBIAN_FRONTEND=

# Use an entrypoint script to source the ROS2 and workspace setup files for when a container starts non interactively.
COPY ./Docker/entrypoint.sh /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]

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
    vim \
    nano \
    htop \
    tree \
    ros-dev-tools

# Install LLVM 18 (for C++ compilation)
RUN wget --progress=dot:giga https://apt.llvm.org/llvm.sh && chmod +x llvm.sh && ./llvm.sh 18 all
RUN update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-18 100 \
    && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 100 \
    && update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 100 \
    && update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-18 100 \
    && update-alternatives --install /usr/bin/run-clang-tidy run-clang-tidy /usr/bin/run-clang-tidy-18 100

ENV CC=/usr/bin/clang-18
ENV CXX=/usr/bin/clang++-18

# Install ROS2 tools and gazebo
RUN apt-get update && apt-get install -y --no-install-recommends \
    ros-$ROS_DISTRO-ros-gz \
    ros-$ROS_DISTRO-gz-ros2-control \
    ros-$ROS_DISTRO-rqt* \
    ros-$ROS_DISTRO-teleop-twist-keyboard \
    ros-$ROS_DISTRO-rviz-satellite \
    unzip \
    default-jre

# Copy all modules so we only have to build once
COPY robotics_modules /$WORKDIR/src/
COPY utilities /$WORKDIR/utilities/
RUN /$WORKDIR/utilities/osm2sdf/osm2sdf_install.sh -g
RUN echo 'export PYTHONPATH=$PYTHONPATH:/workspace' >> ~/.bashrc

# Setup workspace directory
WORKDIR /$WORKDIR

# Update package lists and install dependencies and build
RUN apt-get update \
    && . /opt/ros/${ROS_DISTRO}/setup.sh \
    && rosdep install --from-paths src --ignore-src -r -y \
    && colcon build --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Debug
  
    # Add argcomplete for ROS2 and colcon
    RUN echo "eval \"$(/usr/bin/register-python-argcomplete3 ros2)\"" >> ~/.bashrc \
    && echo "eval \"$(/usr/bin/register-python-argcomplete3 colcon)\"" >> ~/.bashrc
    
    # Add helpful aliases for development
    RUN echo "alias cb='colcon build --symlink-install'" >> ~/.bashrc && \
    echo "alias cbs='colcon build --symlink-install --packages-select'" >> ~/.bashrc && \
    echo "alias ct='colcon test'" >> ~/.bashrc && \
    echo "alias ctr='colcon test-result --verbose'" >> ~/.bashrc && \
    echo "alias resource='source /opt/ros/\${ROS_DISTRO}/setup.bash && source /workspace/install/setup.bash'" >> ~/.bashrc
    
    ENV DEBIAN_FRONTEND=
