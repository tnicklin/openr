# Open/R Dockerized Setup

This repository provides a Dockerized version of Open/R and its dependencies.

## Prerequisites

Ensure you have Docker and Git installed on your machine.

## Clone the Repository

To clone this repository and its submodules:

```bash
git clone --recursive https://github.com/tnicklin/openr  
cd openr
```
If you forgot to clone the submodules:

git submodule update --init --recursive

## Build the Docker Image

To build the Docker image for Open/R:
```bash
docker build -t openr-image .
```
## Configuration

Place your configuration files in the `config` directory. Ensure each node has its own configuration file (e.g., `node_1.conf`, `node_2.conf`, etc.).

## Run the Open/R Container

You can run Open/R with the following command, replacing `node_1.conf` with your configuration file:
```bash
docker run --rm --privileged --network host \
       -v /path/to/config/dir:/config \
       openr-image /scripts/run_openr.sh node_1.conf
```
## Publishing the Control Port

The control port is defined in the Open/R configuration file. You can set an environment variable to map the control port dynamically:
```bash
docker run --rm --privileged --network host \
  -v /path/to/config/dir:/config \
  -e OPENR_CTRL_PORT=2019 \
  openr-image /scripts/run_openr.sh node_1.conf
```
This command ensures that the container uses the `OPENR_CTRL_PORT` environment variable for dynamic port mapping.

## Running Bash in the Container

If you want to start an interactive `bash` session inside the container, use the following command:
```bash
docker run -it --rm openr-image bash
```
