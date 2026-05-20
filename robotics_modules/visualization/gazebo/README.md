# Gazebo

## Gazebo ROS2 package parameters and usage

This package exposes launch arguments to configure simulation startup and robot spawning.

- `world` (string, default: `empty`) - world name (or SDF base name) loaded by `ros_gz_sim`.
- `namespace` (string, default: `''`) - ROS namespace for the spawned robot and bridge nodes.
- `use_sim_time` (bool, default: `True`) - enables simulated time via `/clock`.
- `parameter_bridge_config` (path) - path to bridge YAML, default points to `config/parameter_bridge.yaml` in this package.
- `spawn_x` (float, default: `0.0`) - robot spawn X position.
- `spawn_y` (float, default: `0.0`) - robot spawn Y position.
- `spawn_z` (float, default: `0.0`) - robot spawn Z position.

### Usage examples

Run with default settings:

```bash
ros2 launch gazebo gazebo.launch.py
```

Specify world and spawn namespace:

```bash
ros2 launch gazebo gazebo.launch.py world:=your_world namespace:=robot1
```

Force real time integration off (local clock):

```bash
ros2 launch gazebo gazebo.launch.py use_sim_time:=False
```

If you need full control over robot model and spawn pose, use `gz_spawn_model` directly with the standard `ros_gz_sim` launch files in `ros_gz_sim`.

## Using `<gazebo_ros>` to export model paths in package.xml

From [ros_gz_sim](https://github.com/gazebosim/ros_gz/tree/jazzy/ros_gz_sim)

The `<gazebo_ros>` tag inside the `<export>` tag of a `package.xml` file can be used to add paths to `GZ_SIM_RESOURCE_PATH` and `GZ_SIM_SYSTEM_PLUGIN_PATH`, which are environment variables used to configure Gazebo search paths for resources (e.g., SDFormat files, meshes) and plugins respectively.

### Attributes

- `gazebo_model_path` and `gazebo_media_path`: appended to `GZ_SIM_RESOURCE_PATH`
- `plugin_path`: appended to `GZ_SIM_SYSTEM_PLUGIN_PATH`


### Using `${prefix}` variable

The `${prefix}` keyword expands to the package's share path (i.e., the value of `ros2 pkg prefix --share <package_name>`).

**Example configuration:**

```xml
<export>
    <gazebo_ros gazebo_model_path="${prefix}/models"/>
    <gazebo_ros gazebo_media_path="${prefix}/media"/>
    <gazebo_ros plugin_path="${prefix}/plugins"/>
</export>
```

### Installing directories

Add the required directory installation to `CMakeLists.txt`:

```cmake
install(DIRECTORY models
     DESTINATION share/${PROJECT_NAME})
```

### Recommended practice

To reference models in a ROS package unambiguously, set the `gazebo_model_path` to the parent of the prefix:

```xml
<export>
    <gazebo_ros gazebo_model_path="${prefix}/../"/>
</export>
```
