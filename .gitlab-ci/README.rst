CI Docker Images
================

There are two images which are used for CI and which can be found here:
https://gitlab.gnome.org/GNOME/pygobject/container_registry

* `Dockerfile` - contains various Python versions and a commonly used distro.
  Run `run-docker.sh` to build it and run a shell in it. After that it can be pushed.
* `Dockerfile.old` - 32bit using the oldest supported distro, to test with old stuff.
  Run `run-docker-old.sh` to build it and run a shell in it. After that it can be pushed.

The scripts spawn a shell in the container with the source code mounted, so
things can be tested locally if needed.
