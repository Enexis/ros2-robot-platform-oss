#!/usr/bin/env python3
# src/unitree_go2_controller/launch/controller.launch.py

import os

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, Command, FindExecutable
from launch_ros.actions import Node
import launch_ros.descriptions
from launch_ros.substitutions import FindPackageShare

def launch_setup(context: LaunchConfiguration):
    # Configuration files
    kinematics_config = PathJoinSubstitution([
        FindPackageShare('unitree_go2_controller'),
        'config',
        'kinematics_config.yaml'
    ])

    # Launch arguments
    namespace = LaunchConfiguration('namespace')
    use_sim_time = LaunchConfiguration('use_sim_time')
    control_rate = LaunchConfiguration('control_rate')

    robot_description_content = Command(
        [
            PathJoinSubstitution([FindExecutable(name='xacro')]),
            ' ',
            PathJoinSubstitution([FindPackageShare('go2_description'), 'xacro', 'unitree_go2.xacro']),
            ' ',
            'prefix:=go2_'
        ]
    )
    robot_description_param = launch_ros.descriptions.ParameterValue(robot_description_content,
                                                                     value_type=str)
    
    # Quadruped controller node
    controller_node = Node(
        package='unitree_go2_controller',
        executable='quadruped_controller_node',
        namespace=namespace,
        name='quadruped_controller',
        output='screen',
        parameters=[
            kinematics_config,
            {
                'robot_description': robot_description_param,
                'use_sim_time': use_sim_time,
                'control_rate': control_rate
            }
        ],
        remappings=[
            ('/odom', '/odom1/raw')
        ]
    )
    
    return [controller_node]


def generate_launch_description():

    namespace_arg = DeclareLaunchArgument(
        'namespace',
        default_value='',
        description='Namespace to be added to the robot description.'
    )

    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='true',
        description='Use simulation time'
    )
    
    control_rate_arg = DeclareLaunchArgument(
        'control_rate',
        default_value='50.0',
        description='Controller update rate (Hz)'
    )

    return LaunchDescription([
        namespace_arg,
        use_sim_time_arg,
        control_rate_arg,
        OpaqueFunction(function=launch_setup)
    ])
