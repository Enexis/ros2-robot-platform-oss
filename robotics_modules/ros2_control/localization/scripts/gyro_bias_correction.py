#!/usr/bin/env python3
"""
Republish an IMU with the gyro offset removed.

The offset is learned only while the robot stands still: low gyro noise over
the last window and no non-zero velocity command for a while. The Livox IMU
reports acceleration in g; it is converted to m/s^2.
"""

from collections import deque
import math

from geometry_msgs.msg import Twist
import rclpy
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import Imu

GRAVITY = 9.80665


def window_stats(samples):
    """Return per-axis mean and standard deviation of (x, y, z) samples."""
    n = len(samples)
    mean = [sum(s[i] for s in samples) / n for i in range(3)]
    std = [math.sqrt(sum((s[i] - mean[i]) ** 2 for s in samples) / n) for i in range(3)]
    return mean, std


class GyroBiasCorrection(Node):
    """Estimate the gyro offset at standstill and subtract it."""

    def __init__(self):
        super().__init__('gyro_bias_correction')
        self._window = float(self.declare_parameter('window', 1.0).value)
        self._still_std = float(self.declare_parameter('still_std', 0.01).value)
        self._max_bias = float(self.declare_parameter('max_bias', 0.05).value)
        self._cmd_quiet = float(self.declare_parameter('cmd_quiet_time', 1.5).value)
        self._time_constant = float(self.declare_parameter('time_constant', 5.0).value)
        self._accel_scale = float(self.declare_parameter('accel_scale', GRAVITY).value)
        self._gyro_var = float(self.declare_parameter('gyro_variance', 1e-4).value)
        self._samples = deque()
        self._bias = None
        self._last_update = None
        self._last_motion_cmd = None
        self._pub = self.create_publisher(Imu, 'imu_out', qos_profile_sensor_data)
        self.create_subscription(Imu, 'imu_in', self._on_imu, qos_profile_sensor_data)
        self.create_subscription(Twist, 'cmd_vel', self._on_cmd, 10)

    def _now(self):
        return self.get_clock().now().nanoseconds * 1e-9

    def _on_cmd(self, msg):
        if any(abs(v) > 1e-3 for v in (msg.linear.x, msg.linear.y, msg.angular.z)):
            self._last_motion_cmd = self._now()

    def _update_bias(self, now):
        mean, std = window_stats([s[1:] for s in self._samples])
        if max(std) > self._still_std or max(abs(m) for m in mean) > self._max_bias:
            return
        if self._last_motion_cmd is not None and now - self._last_motion_cmd < self._cmd_quiet:
            return
        if self._bias is None:
            self._bias = mean
            self.get_logger().info(
                'Gyro offset estimated: ' + ', '.join(f'{math.degrees(b):+.3f}' for b in mean)
                + ' deg/s')
        else:
            alpha = min(1.0, (now - self._last_update) / self._time_constant)
            self._bias = [b + alpha * (m - b) for b, m in zip(self._bias, mean)]
        self._last_update = now

    def _on_imu(self, msg):
        now = self._now()
        w = msg.angular_velocity
        self._samples.append((now, w.x, w.y, w.z))
        while self._samples and now - self._samples[0][0] > self._window:
            self._samples.popleft()
        if (now - self._samples[0][0] >= 0.9 * self._window
                and (self._last_update is None or now - self._last_update >= 0.1)):
            self._update_bias(now)

        if self._bias is not None:
            w.x -= self._bias[0]
            w.y -= self._bias[1]
            w.z -= self._bias[2]
        a = msg.linear_acceleration
        a.x *= self._accel_scale
        a.y *= self._accel_scale
        a.z *= self._accel_scale
        if not any(msg.angular_velocity_covariance):
            msg.angular_velocity_covariance[0] = self._gyro_var
            msg.angular_velocity_covariance[4] = self._gyro_var
            msg.angular_velocity_covariance[8] = self._gyro_var
        msg.orientation_covariance[0] = -1.0
        self._pub.publish(msg)


def main():
    rclpy.init()
    node = GyroBiasCorrection()
    try:
        rclpy.spin(node)
    except (KeyboardInterrupt, ExternalShutdownException):
        pass
    finally:
        node.destroy_node()
        rclpy.try_shutdown()


if __name__ == '__main__':
    main()
