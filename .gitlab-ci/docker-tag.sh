#!/bin/bash

set -e

# This script runs on the buildah image, which is Fedora-based
sudo dnf install -y skopeo

echo "$CI_REGISTRY_PASSWORD" | skopeo login "$CI_REGISTRY" -u "$CI_REGISTRY_USER" --password-stdin

skopeo copy "docker://$1" "docker://$2"
