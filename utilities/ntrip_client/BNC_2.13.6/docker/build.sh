#!/bin/bash

# Build BNC from sources for multiple operating systems
# On redhat/centos install "dnf install podman-docker"
# Wiese, 2022

set -e
cd "$(dirname $0)/.."

bncvers=$(grep "#define BNCVERSION \"" src/bncversion.h | cut -d " " -f3 | tr -d \")
if [ -z "$bncvers" ]; then
  echo "ERROR: could not find bncvers"; exit 1
fi

# Run image, copy the compiled executable "bncnew" from the container
# and then remove the container.
function getBncFromImage () {
  local image="$1"
  local imgID=$(docker images --format "{{.ID}}" $image)
  local con=$(docker create $imgID)
  docker cp $con:/tmp/BNC/bnc bncnew
  docker rm $con
  [ -s "bncnew" ] || { echo "executable \"bncnew\" does not exist"; exit 1; }
}

function buildBullseye () {
  echo "Build BNC $bncvers for debian 11 (bullseye) ..."
  [ -f "bncnew" ] && rm bncnew
  docker build -t bullseye/bnc:v$bncvers -f docker/Dockerfile.bullseye .
  getBncFromImage "bullseye/bnc:v$bncvers"
  target=$(realpath "bnc-$bncvers-debian11")
  mv bncnew $target
  echo "$target created"
}

function buildBookworm () {
  echo "Build BNC $bncvers for debian 12 (bookworm) ..."
  [ -f "bncnew" ] && rm bncnew
  docker build -t bookworm/bnc:v$bncvers -f docker/Dockerfile.bookworm .
  getBncFromImage "bookworm/bnc:v$bncvers"
  target=$(realpath "bnc-$bncvers-debian12")
  mv bncnew $target
  echo "$target created"
}

function buildTrixie () {
  echo "Build BNC $bncvers for debian 13 (trixie) ..."
  [ -f "bncnew" ] && rm bncnew
  docker build -t trixie/bnc:v$bncvers -f docker/Dockerfile.trixie .
  getBncFromImage "trixie/bnc:v$bncvers"
  target=$(realpath "bnc-$bncvers-debian13")
  mv bncnew $target
  echo "$target created"
}

function buildLeap () {
  echo "Build BNC $bncvers for opensuse (leap) ..."
  [ -f "bncnew" ] && rm bncnew
  docker build -t leap/bnc:v$bncvers -f docker/Dockerfile.leap .
  getBncFromImage "leap/bnc:v$bncvers"
  target=$(realpath "bnc-$bncvers-leap")
  mv bncnew $target
  echo "$target created"
}

function buildUbi8 () {
  echo "Build BNC $bncvers for redhat/ubi8 ..."
  [ -f "bncnew" ] && rm bncnew
  docker build -t ubi8.6/bnc:v$bncvers -f docker/Dockerfile.ubi8 .
  getBncFromImage "ubi8.6/bnc:v$bncvers"
  target=$(realpath "bnc-$bncvers-el8.6")
  mv bncnew $target
  echo "$target created"
}

# rockylinux 9/el9
function buildRocky () {
  echo "Build BNC $bncvers for rockylinux ..."
  [ -f "bncnew" ] && rm bncnew
  docker build -t rocky9/bnc:v$bncvers -f docker/Dockerfile.rocky9 .
  getBncFromImage "rocky9/bnc:v$bncvers"
  target=$(realpath "bnc-$bncvers-rocky9")
  mv bncnew $target
  echo "$target created"
}

case $1 in
     "bullseye")
         buildBullseye;
         ;;
     "bookworm")
         buildBookworm;
         ;;
      "trixie")
         buildTrixie;
         ;;
     "leap")
         buildLeap;
         ;;
     "ubi")
         buildUbi8;
         ;;
     "rocky")
         buildRocky;
         ;;
     "all")
         buildBookworm;
         buildLeap;
         buildUbi8;
         ;;
     *)
         echo "Usage:"
         echo "    $0 { bullseye | leap | ubi | rocky | all }"
         echo
         exit 1;
         ;;
esac

