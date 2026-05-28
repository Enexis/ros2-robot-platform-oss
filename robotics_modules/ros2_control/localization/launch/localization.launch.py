import os

from launch import LaunchContext, LaunchDescription
from launch.actions import OpaqueFunction, DeclareLaunchArgument
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
    
    ekf_local_config = PathJoinSubstitution([
        FindPackageShare('localization'),
        'config',
        'ekf_local.yaml'
    ])

    ekf_global_config = PathJoinSubstitution([
        FindPackageShare('localization'),
        'config',
        'ekf_global.yaml'
    ])

    navsat_config = PathJoinSubstitution([
        FindPackageShare('localization'),
        'config',
        'navsat_config.yaml'
    ])
    
    ekf_local = Node(
        package="robot_localization",
        executable="ekf_node",
        name="ekf_local",
        namespace=namespace,
        parameters=[
            ekf_local_config,
            {
                "use_sim_time": LaunchConfiguration("use_sim_time")
            }
        ],
        remappings=[('odometry/filtered', 'odometry/local')],
    )

    ekf_global = Node(
        package="robot_localization",
        executable="ekf_node",
        name="ekf_global",
        namespace=namespace,
        parameters=[
            ekf_global_config,
            {
                "use_sim_time": LaunchConfiguration("use_sim_time")
            }
        ],
        remappings=[('odometry/filtered', 'odometry/global')] 
    )

    navsat_transform = Node(
        package='robot_localization', 
        executable='navsat_transform_node', 
        name='navsat_transform',
        output='screen',
        parameters=[navsat_config, {"use_sim_time": LaunchConfiguration("use_sim_time")}],
        remappings=[
                    ('imu', 'imu'),
                    ('odometry/filtered', 'odometry/global')
                    # ('gps/fix', 'gps/fix'), 
                    # ('gps/filtered', 'gps/filtered'),
                    # ('odometry/gps', 'odometry/gps'),
                ]           
    )           

    return [ekf_local, ekf_global, navsat_transform]


def generate_launch_description() -> LaunchDescription:
    """Generate the launch description.

    Returns:
        LaunchDescription: The launch description.
    """

    declare_namespace_cmd = DeclareLaunchArgument(
        'namespace',
        default_value='',
        description='Namespace for the robot'
    )
    declare_sim_time_cmd = DeclareLaunchArgument(
        'use_sim_time',
        default_value='True',
        description='Use sim time for the robot'
    )

    return LaunchDescription(
        [
            declare_namespace_cmd,
            declare_sim_time_cmd,
            OpaqueFunction(function=launch_setup),
        ]
    )
