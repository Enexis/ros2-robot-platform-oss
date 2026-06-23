# Build BNC from sources for multiple operating systems

Docker or podman must be installed on your system, see [Podman Installation Instructions](https://podman.io/getting-started/installation).<br>
The Dockerfiles should work with both commands, docker and podman.

Build example:

```bash
cd to/BNC/root/dir
podman build -t bullseye/bnc:v2.13 -f docker/Dockerfile.bullseye .
```

#### Copy the build artifact from the container
```bash
con=$(podman create <imageID>)
podman cp $con:/tmp/BNC/bnc bncnew
podman rm $con
```