# OSM2SDF – Open Street Map to Gazebo World Pipeline

A small toolchain to generate **Gazebo (GZ Sim) SDF worlds** from **OpenStreetMap (OSM)** data around a given geographic location (latitude & longitude).

It includes:

* A **setup script** to install dependencies and OSM2World
* A Python script **`osm2sdf.py`** to download map data and generate an SDF world
* Automatic configuration of required environment variables for Gazebo

***

## Contents

```text
.
├── setup.sh          # Setup & installation script
├── osm2sdf.py        # Main map-to-SDF conversion script
├── osm2world/        # Downloaded OSM2World binaries (auto-created)
├── tmp/              # Gazebo resource directory (auto-created)
└── README.md
```

***

## Requirements

* Ubuntu / Debian-based system
* `bash`
* Root or `sudo` access (for `apt`)
* `wget`
* `python3`
* `unzip`
* Java runtime to run OSM2WORLD eg `default-jre`
  
***

## Setup Script

### Purpose

The setup script:

* Downloads and installs **OSM2World**
* Installs required system dependencies
* Installs Python dependencies (globally or via virtual environment)
* Configures environment variables in `~/.bashrc`

***

### Usage

```bash
./setup.sh [-g | -v]
```

#### Options

| Option | Description                                                                  |
| ------ | ---------------------------------------------------------------------------- |
| `-g`   | Install Python dependencies **globally** using `apt`                         |
| `-v`   | Create a **local Python virtual environment** and install dependencies there |

If no or an invalid option is provided, a usage message is shown and the script exits.

***

## `osm2sdf.py`

`osm2sdf.py` generates a Gazebo world from OpenStreetMap data centered on a given latitude and longitude. Call it with `create_map_worldlat: float, lon: float)` or from command line with `python3 osm2sdf.py <lat> <lon>`

Internally, it performs three steps:

1. Download OSM map data
2. Convert the map using OSM2World
3. Generate an SDF world file for Gazebo

***

### Example

<video  controls>
  <source src="OSM2SDF.mp4" type="video/mp4">
</video>

note: Screenrecordings of WSL2 cause visual artifacts that aren't there in practice.

```bash
# python3 osm2sdf.py <lat> <lon>
python3 osm2sdf.py 51.708276 5.299235
```

```python
# In this case osm2sdf lives in a utilities folder make sure it or a parent folder is added to the PYTHONPATH env variable
from utilities.osm2sdf.osm2sdf import create_map_world
create_map_world(lat=51.708276, lon=5.299235)
```

This generates a Gazebo-ready SDF world centered around **Orthen Den Bosch**.

***

## Running the Generated World in Gazebo

Once the SDF world is generated and resources are placed in `tmp/`:

```bash
gz sim generated_world.sdf
```

Gazebo will automatically resolve models and resources via `GZ_SIM_RESOURCE_PATH`.

If you want to keep the world you will need to copy the `.sdf` and `.glb` files from the `tmp` folder.


## Acknowledgements / Attribution

This utility includes and extends code from [alliander-robotics]
(https://github.com/alliander-opensource/alliander-robotics/), licensed under the Apache License 2.0.

Modifications:
- Refactored create_sdf.py to osm2sdf.py

Original copyright:
Copyright 2025 Alliander
