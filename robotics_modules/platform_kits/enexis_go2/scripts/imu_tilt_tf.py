#!/usr/bin/env python3
"""
Publish base_footprint -> base_link with the body roll and pitch from the IMU.

base_footprint stays level (the local EKF runs in 2D), so heights in it are
measured against gravity even while the Go2 body pitches and rolls.
"""

import math

from geometry_msgs.msg import TransformStamped
import rclpy
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import Imu
from tf2_ros import TransformBroadcaster


def tilt_quaternion(x, y, z, w):
    """Return (x, y, z, w) of the roll and pitch of a quaternion, with yaw removed."""
    roll = math.atan2(2.0 * (w * x + y * z), 1.0 - 2.0 * (x * x + y * y))
    pitch = math.asin(max(-1.0, min(1.0, 2.0 * (w * y - z * x))))
    cr, sr = math.cos(roll / 2.0), math.sin(roll / 2.0)
    cp, sp = math.cos(pitch / 2.0), math.sin(pitch / 2.0)
    return (sr * cp, cr * sp, -sr * sp, cr * cp)


class ImuTiltTf(Node):

    def __init__(self):
        super().__init__('imu_tilt_tf')
        self._parent = self.declare_parameter('parent_frame', 'base_footprint').value
        self._child = self.declare_parameter('child_frame', 'base_link').value
        self._period = 1.0 / float(self.declare_parameter('publish_rate', 50.0).value)
        self._last = None
        self._broadcaster = TransformBroadcaster(self)
        self.create_subscription(Imu, 'imu', self._on_imu, qos_profile_sensor_data)

    def _on_imu(self, msg):
        q = msg.orientation
        if q.x == q.y == q.z == q.w == 0.0:
            return
        stamp = msg.header.stamp.sec + msg.header.stamp.nanosec * 1e-9
        if self._last is not None and 0.0 <= stamp - self._last < self._period:
            return
        self._last = stamp
        tf = TransformStamped()
        tf.header.stamp = msg.header.stamp
        tf.header.frame_id = self._parent
        tf.child_frame_id = self._child
        r = tf.transform.rotation
        r.x, r.y, r.z, r.w = tilt_quaternion(q.x, q.y, q.z, q.w)
        self._broadcaster.sendTransform(tf)


def main():
    rclpy.init()
    node = ImuTiltTf()
    try:
        rclpy.spin(node)
    except (KeyboardInterrupt, ExternalShutdownException):
        pass
    finally:
        node.destroy_node()
        rclpy.try_shutdown()


if __name__ == '__main__':
    main()
