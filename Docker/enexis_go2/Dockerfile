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

COPY robotics_modules/ros2_control/localization /$WORKDIR/src/localization

# Build the workspace with the specified packages and clean up to reduce image size
RUN . /opt/ros/${ROS_DISTRO}/setup.sh && \
    rosdep install --from-paths src --ignore-src -r -y && \
    colcon build && \
    rm -rf /var/lib/apt/lists/*
    
    # rm -rf build log src && \
ENV DEBIAN_FRONTEND=
