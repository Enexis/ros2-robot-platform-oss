import os

from launch import LaunchContext, LaunchDescription
from launch.actions import OpaqueFunction, DeclareLaunchArgument
from launch.conditions import IfCondition
from launch_ros.actions import Node, SetParameter
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_directory


def launch_setup(context: LaunchContext) -> list:
    """The launch setup.

    Args:
        context (LaunchContext): The launch context.

    Returns:
        list: The actions to start.
    """
    namespace = LaunchConfiguration("namespace")
    ekf_local_config = LaunchConfiguration("ekf_local_config")
    ekf_global_config = LaunchConfiguration("ekf_global_config")
    navsat_config = LaunchConfiguration("navsat_config")
    livox_gyro = LaunchConfiguration("livox_gyro").perform(context).lower() in ("true", "1")

    ekf_local_overrides = {"use_sim_time": LaunchConfiguration("use_sim_time")}
    if livox_gyro:
        # Yaw rate from the Livox gyro (offset removed). Its TF to base_footprint
        # includes the IMU-driven body tilt, so wait briefly for that transform.
        ekf_local_overrides.update({"imu0": "livox/imu_corrected", "transform_timeout": 0.05})

    gyro_bias_correction = Node(
        package="localization",
        executable="gyro_bias_correction.py",
        name="gyro_bias_correction",
        namespace=namespace,
        output="screen",
        remappings=[("imu_in", "livox/imu"), ("imu_out", "livox/imu_corrected")],
        ros_arguments=["--log-level", "rmw_cyclonedds_cpp:=error"],
        condition=IfCondition(str(livox_gyro)),
    )

    ekf_local = Node(
        package="robot_localization",
        executable="ekf_node",
        name="ekf_local",
        namespace=namespace,
        parameters=[
            ekf_local_config,
            ekf_local_overrides,
        ],
        remappings=[("odometry/filtered", "odometry/local")],
        ros_arguments=["--log-level", "rmw_cyclonedds_cpp:=error"],
        condition=IfCondition(LaunchConfiguration("start_ekf_local")),
    )

    ekf_global = Node(
        package="robot_localization",
        executable="ekf_node",
        name="ekf_global",
        namespace=namespace,
        parameters=[
            ekf_global_config,
            {"use_sim_time": LaunchConfiguration("use_sim_time")},
        ],
        remappings=[("odometry/filtered", "odometry/global")],
        ros_arguments=["--log-level", "rmw_cyclonedds_cpp:=error"],
        condition=IfCondition(LaunchConfiguration("start_ekf_global")),
    )

    navsat_transform = Node(
        package="robot_localization",
        executable="navsat_transform_node",
        name="navsat_transform",
        namespace=namespace,
        output="screen",
        parameters=[
            navsat_config,
            {"use_sim_time": LaunchConfiguration("use_sim_time")},
        ],
        remappings=[
            ("imu", "imu"),
            ("odometry/filtered", "odometry/global"),
            ('gps/fix', 'navsatfix'),
            # ('gps/filtered', 'gps/filtered'),
            # ('odometry/gps', 'odometry/gps'),
        ],
        ros_arguments=["--log-level", "rmw_cyclonedds_cpp:=error"],
        condition=IfCondition(LaunchConfiguration("start_navsat_transform")),
    )

    return [gyro_bias_correction, ekf_local, ekf_global, navsat_transform]


def generate_launch_description() -> LaunchDescription:
    """Generate the launch description.

    Returns:
        LaunchDescription: The launch description.
    """

    declare_namespace_cmd = DeclareLaunchArgument(
        "namespace", default_value="", description="Namespace for the robot"
    )
    declare_sim_time_cmd = DeclareLaunchArgument(
        "use_sim_time", default_value="False", description="Use sim time for the robot"
    )
    declare_ekf_local_config_cmd = DeclareLaunchArgument(
        "ekf_local_config",
        default_value=PathJoinSubstitution(
            [FindPackageShare("localization"), "config", "ekf_local.yaml"]
        ),
        description="Path to the EKF local config file",
    )
    declare_ekf_global_config_cmd = DeclareLaunchArgument(
        "ekf_global_config",
        default_value=PathJoinSubstitution(
            [FindPackageShare("localization"), "config", "ekf_global.yaml"]
        ),
        description="Path to the EKF global config file",
    )
    declare_navsat_config_cmd = DeclareLaunchArgument(
        "navsat_config",
        default_value=PathJoinSubstitution(
            [FindPackageShare("localization"), "config", "navsat_config.yaml"]
        ),
        description="Path to the navsat transform config file",
    )
    declare_start_ekf_local_cmd = DeclareLaunchArgument(
        "start_ekf_local", 
        default_value="True",
        description="Start local EKF node"
    )
    declare_start_ekf_global_cmd = DeclareLaunchArgument(
        "start_ekf_global",
        default_value="True",
        description="Start global EKF node"
    )
    declare_start_navsat_transform_cmd = DeclareLaunchArgument(
        "start_navsat_transform",
        default_value="True",
        description="Start navsat transform node",
    )
    declare_livox_gyro_cmd = DeclareLaunchArgument(
        "livox_gyro",
        default_value="False",
        description="Local EKF yaw rate from the Livox gyro (offset-corrected) instead of /imu",
    )

    return LaunchDescription(
        [
            declare_namespace_cmd,
            declare_sim_time_cmd,
            declare_ekf_local_config_cmd,
            declare_ekf_global_config_cmd,
            declare_navsat_config_cmd,
            declare_start_ekf_local_cmd,
            declare_start_ekf_global_cmd,
            declare_start_navsat_transform_cmd,
            declare_livox_gyro_cmd,
            OpaqueFunction(function=launch_setup),
        ]
    )
