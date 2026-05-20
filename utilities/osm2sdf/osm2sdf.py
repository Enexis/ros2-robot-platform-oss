# SPDX-FileCopyrightText: Alliander N. V.
#
# SPDX-License-Identifier: Apache-2.0


import subprocess
import xmltodict
from pyproj import Geod
from termcolor import colored
import os
import sys
from pathlib import Path

try:
    from rclpy.logging import get_logger
    logger = get_logger("osm2sdf")
except ImportError:
    import logging
    logging.basicConfig(level=logging.INFO)
    logger = logging.getLogger("osm2sdf")

generated_world_path = Path(os.environ["OSM2SDF_PATH"])


def create_map_world(lat: float, lon: float) -> None:
    """Create a map world SDF file based on the given longitude and latitude.

    Args:
        lat (float): The latitude of the map center.
        lon (float): The longitude of the map center.
    """
    download_map(lat, lon)
    convert_map()
    create_sfd(lat, lon)

def download_map(lat: float, lon: float) -> None:
    """Download map data from OpenStreetMap for the specified longitude and latitude.

    Args:
        lat (float): The latitude of the map center.
        lon (float): The longitude of the map center.
    """
    logger.info(colored("Downloading map data from OpenStreetMap...", "white"))
    geo = Geod(ellps="WGS84")
    distance = 100  # meters

    lats = []
    longs = []
    angle = 45

    for _ in range(4):
        corner = geo.fwd(lon, lat, angle, distance)
        longs.append(corner[0])
        lats.append(corner[1])
        angle += 90

    min_lon = min(longs)
    max_lon = max(longs)
    min_lat = min(lats)
    max_lat = max(lats)

    cmd_download = [
        "wget",
        "-O",
        f"{generated_world_path}/tmp/map.osm",
        f"https://api.openstreetmap.org/api/0.6/map?bbox={min_lon},{min_lat},{max_lon},{max_lat}",
    ]
    subprocess.run(cmd_download, check=False, stderr=subprocess.DEVNULL)


def convert_map() -> None:
    """Convert the downloaded OSM map data to glb format."""
    logger.info(colored("Converting map data to glb format...", "white"))

    cmd_convert = [
        "bash",
        f"{generated_world_path}/osm2world/osm2world.sh",
        "convert",
        "--config",
        f"{generated_world_path}/osm2world/standard.properties",
        "-i",
        f"{generated_world_path}/tmp/map.osm",
        "-o",
        f"{generated_world_path}/tmp/map.glb",
    ]
    subprocess.run(cmd_convert, check=False)


def create_sfd(lat: float, lon: float) -> None:
    """Create an SDF world file from the glb map object.

    Args:
        lat (float): The latitude of the map center.
        lon (float): The longitude of the map center.
    """
    logger.info(colored("Creating SDF file from glb object...", "white"))
    with open(f"{generated_world_path}/unset_world.sdf", encoding="utf-8") as fd:
        sdf_string = fd.read()

    sdf_dict = xmltodict.parse(sdf_string)
    sdf_dict["sdf"]["world"]["spherical_coordinates"]["longitude_deg"] = lon
    sdf_dict["sdf"]["world"]["spherical_coordinates"]["latitude_deg"] = lat

    with open(f"{generated_world_path}/tmp/generated_world.sdf", "w", encoding="utf-8") as fd:
        fd.write(xmltodict.unparse(sdf_dict))

if __name__ == '__main__':
    lat = float(sys.argv[1])
    lon = float(sys.argv[2])
    create_map_world(lat, lon)