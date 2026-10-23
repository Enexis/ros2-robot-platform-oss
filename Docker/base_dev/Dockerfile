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
# Package manifests only. The dev rosdep layer mounts these, so it is
# reused until a package.xml changes instead of on every source change.
###########################################
FROM base AS manifests
COPY robotics_modules /tmp/src
RUN mkdir /manifests && cd /tmp/src && \
    find . -name package.xml -exec cp --parents {} /manifests/ \;

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

# Install osm2sdf separately so its layer is only invalidated when osm2sdf itself changes
COPY utilities/osm2sdf /$WORKDIR/utilities/osm2sdf/
RUN /$WORKDIR/utilities/osm2sdf/osm2sdf_install.sh -g

RUN echo 'export PYTHONPATH=$PYTHONPATH:/workspace' >> ~/.bashrc

# Setup workspace directory
WORKDIR /$WORKDIR

# System dependencies of all packages, resolved from their package.xml files only.
# Reused until a package.xml changes instead of on every source change.
RUN --mount=type=bind,from=manifests,source=/manifests,target=/tmp/manifests \
    apt-get update && \
    . /opt/ros/${ROS_DISTRO}/setup.sh && \
    rosdep install --from-paths /tmp/manifests --ignore-src -r -y

# Compiled packages that change rarely
COPY robotics_modules/application_control /$WORKDIR/src/application_control/
COPY robotics_modules/mobile_bases /$WORKDIR/src/mobile_bases/
COPY robotics_modules/sensors /$WORKDIR/src/sensors/
RUN . /opt/ros/${ROS_DISTRO}/setup.sh && \
    colcon build --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Debug

# Launch/config packages that change often; a change here only rebuilds this layer
COPY robotics_modules/platform_kits /$WORKDIR/src/platform_kits/
COPY robotics_modules/ros2_control /$WORKDIR/src/ros2_control/
COPY robotics_modules/visualization /$WORKDIR/src/visualization/
RUN . /opt/ros/${ROS_DISTRO}/setup.sh && \
    colcon build --symlink-install --packages-skip-build-finished --cmake-args -DCMAKE_BUILD_TYPE=Debug

# Not needed by the colcon build; copied last so changes here do not rebuild the workspace
COPY utilities /$WORKDIR/utilities/

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
