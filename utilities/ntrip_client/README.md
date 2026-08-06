# BKG NTRIP Client
NTRIP was developed by the German Federal Agency for Cartography and Geodesy (BKG).
BKG provides a feature-rich open-source (GPL v2) BKG NTRIP Client [BNC](https://igs.bkg.bund.de/ntrip/download).

The docker-compose will build the BNC program using a build container provided by BKG, it is set to debian:bullseye by default but this can be changed depending on the platform required. From that image it is then copied to a small ntrip_client image for running.

```bash
# Open the bnc with ui for configuration of the config file
docker compose --profile config up
# Run bnc without ui according to the config file
docker compose up
```