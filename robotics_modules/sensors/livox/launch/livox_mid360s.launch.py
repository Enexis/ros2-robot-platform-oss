import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, TextSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description() -> LaunchDescription:
    default_config = os.path.join(
        get_package_share_directory("livox"),
        "config",
        "MID360s_config.json",
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "config_path",
                default_value=TextSubstitution(text=default_config),
                description="Path to Livox MID360s json config.",
            ),
            DeclareLaunchArgument(
                "frame_id",
                default_value=TextSubstitution(text="lidar_link"),
                description="Frame ID for published cloud and IMU.",
            ),
            DeclareLaunchArgument(
                "cloud_topic",
                default_value=TextSubstitution(text="livox/lidar"),
                description="Output topic for Livox PointCloud2.",
            ),
            DeclareLaunchArgument(
                "imu_topic",
                default_value=TextSubstitution(text="livox/imu"),
                description="Output topic for Livox IMU.",
            ),
            DeclareLaunchArgument(
                "publish_freq",
                default_value=TextSubstitution(text="10.0"),
                description="Point cloud publish frequency in Hz.",
            ),
            DeclareLaunchArgument(
                "multi_topic",
                default_value=TextSubstitution(text="0"),
                description="0 = shared topic, 1 = per-lidar topics.",
            ),
            Node(
                package="livox_ros_driver2",
                executable="livox_ros_driver2_node",
                name="livox_lidar_publisher",
                output="screen",
                parameters=[
                    {"xfer_format": 0},
                    {
                        "multi_topic": ParameterValue(
                            LaunchConfiguration("multi_topic"), value_type=int
                        )
                    },
                    {"data_src": 0},
                    {
                        "publish_freq": ParameterValue(
                            LaunchConfiguration("publish_freq"), value_type=float
                        )
                    },
                    {"output_data_type": 0},
                    {"frame_id": LaunchConfiguration("frame_id")},
                    {"user_config_path": LaunchConfiguration("config_path")},
                    {"cmdline_input_bd_code": "livox0000000001"},
                ],
                remappings=[
                    ("livox/lidar", LaunchConfiguration("cloud_topic")),
                    ("livox/imu", LaunchConfiguration("imu_topic")),
                ],
            ),
        ]
    )
