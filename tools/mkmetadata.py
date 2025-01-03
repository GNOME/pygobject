import tomllib

from pyproject_metadata import StandardMetadata


def update_metadata_in():
    with open("pyproject.toml", "rb") as fd:
        pyproject = tomllib.load(fd)

    metadata = StandardMetadata.from_pyproject(pyproject, allow_extra_keys=False)
    metadata.version = "@VERSION@"
    del metadata.readme
    del metadata.dependencies[:]

    with open("METADATA.in", "wb") as fd:
        fd.write(metadata.as_rfc822().as_bytes())


if __name__ == "__main__":
    update_metadata_in()
