# Copyright (c) 2024 Intelligent Robotics Lab (URJC)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch_ros.actions import Node
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PythonExpression


def generate_launch_description():
    rviz = LaunchConfiguration("rviz")
    use_sim_time = LaunchConfiguration("use_sim_time")

    declare_namespace_cmd = DeclareLaunchArgument(
        "namespace", default_value="", description="Namespace for the robot"
    )

    declare_rviz_cmd = DeclareLaunchArgument(
        "rviz", default_value="False", description="Launch rviz"
    )

    robot_description_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            [
                os.path.join(get_package_share_directory("go2_description"), "launch/"),
                "robot.launch.py",
            ]
        ),
        launch_arguments={"namespace": LaunchConfiguration("namespace")}.items(),
    )

    # When not simulating use the driver that interacts with the unitree go2 hardware
    phycical_driver_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            [
                os.path.join(get_package_share_directory("go2_driver"), "launch/"),
                "go2_driver.launch.py",
            ]
        ),
        launch_arguments={"namespace": LaunchConfiguration("namespace")}.items(),
        condition=IfCondition(PythonExpression(["not ", use_sim_time])),
    )

    # When simulating use a CHAMP unitree go2 controller with ROS2 controllers
    sim_controller_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            [
                os.path.join(
                    get_package_share_directory("unitree_go2_controller"), "launch/"
                ),
                "controller.launch.py",
            ]
        ),
        launch_arguments={"namespace": LaunchConfiguration("namespace")}.items(),
        condition=IfCondition(PythonExpression([use_sim_time])),
    )

    joint_state_broadcaster_spawner = Node(
        package="controller_manager",
        executable="spawner",
        namespace=LaunchConfiguration("namespace"),
        arguments=["joint_state_broadcaster"],
        condition=IfCondition(PythonExpression([use_sim_time])),
    )

    joint_trajectory_controller_spawner = Node(
        package="controller_manager",
        executable="spawner",
        namespace=LaunchConfiguration("namespace"),
        arguments=["joint_group_effort_controller"],
        condition=IfCondition(PythonExpression([use_sim_time])),
    )
    ############################################################################

    rviz_cmd = Node(
        package="rviz2",
        executable="rviz2",
        name="rviz2",
        output="screen",
        parameters=[{"use_sim_time": use_sim_time}],
        arguments=[
            "-d",
            os.path.join(
                get_package_share_directory("go2_description"),
                "config",
                "go2_rviz.rviz",
            ),
        ],
        condition=IfCondition(PythonExpression([rviz])),
    )

    ld = LaunchDescription()
    ld.add_action(declare_namespace_cmd)
    ld.add_action(declare_rviz_cmd)
    ld.add_action(robot_description_cmd)
    ld.add_action(phycical_driver_cmd)
    ld.add_action(sim_controller_cmd)
    ld.add_action(joint_state_broadcaster_spawner)
    ld.add_action(joint_trajectory_controller_spawner)
    ld.add_action(rviz_cmd)

    return ld
