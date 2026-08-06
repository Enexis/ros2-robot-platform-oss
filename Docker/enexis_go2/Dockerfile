###########################################
# Go2 robot image with packages
###########################################
FROM enexis/base:latest AS go2
ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /$WORKDIR

COPY robotics_modules/platform_kits/enexis_go2 /$WORKDIR/src/enexis_go2
COPY robotics_modules/mobile_bases/unitree_go2 /$WORKDIR/src/unitree_go2
COPY robotics_modules/sensors/sim_gnss /$WORKDIR/src/sim_gnss
COPY robotics_modules/sensors/sim_lidar /$WORKDIR/src/sim_lidar
COPY robotics_modules/sensors/septentrio /$WORKDIR/src/septentrio
COPY robotics_modules/sensors/livox /$WORKDIR/src/livox

COPY robotics_modules/ros2_control/localization /$WORKDIR/src/localization
COPY robotics_modules/ros2_control/nav2 /$WORKDIR/src/nav2

# Build the workspace with the specified packages and clean up to reduce image size
RUN apt-get update && \
    apt-get install -y --no-install-recommends git build-essential cmake && \
    git clone --depth 1 https://github.com/Livox-SDK/Livox-SDK2.git /tmp/Livox-SDK2 && \
    cmake -S /tmp/Livox-SDK2 -B /tmp/Livox-SDK2/build && \
    cmake --build /tmp/Livox-SDK2/build -j$(nproc) && \
    cmake --install /tmp/Livox-SDK2/build && \
    rm -rf /tmp/Livox-SDK2 && \
    git clone --depth 1 https://github.com/Livox-SDK/livox_ros_driver2.git /$WORKDIR/src/livox_ros_driver2 && \
    cp /$WORKDIR/src/livox_ros_driver2/package_ROS2.xml /$WORKDIR/src/livox_ros_driver2/package.xml && \
    ldconfig && \
    . /opt/ros/${ROS_DISTRO}/setup.sh && \
    rosdep install --from-paths src --ignore-src -r -y && \
    colcon build --cmake-args -DDISTRO_ROS=${ROS_DISTRO} && \
    rm -rf /var/lib/apt/lists/*
    
    # rm -rf build log src && \
ENV DEBIAN_FRONTEND=
