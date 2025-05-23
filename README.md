# libloot

![CI](https://github.com/loot/libloot/actions/workflows/ci.yml/badge.svg?branch=master&event=push)
[![Documentation Status](https://readthedocs.org/projects/loot-api/badge/?version=latest)](http://loot-api.readthedocs.io/en/latest/?badge=latest)

## Introduction

LOOT is a plugin load order optimisation tool for Starfield, TES III: Morrowind, TES IV: Oblivion, TES IV: Oblivion Remastered, TES V: Skyrim, TES V: Skyrim Special Edition, TES V: Skyrim VR, Fallout 3, Fallout: New Vegas, Fallout 4, Fallout 4 VR and OpenMW. It is designed to assist mod users in avoiding detrimental conflicts, by automatically calculating a load order that satisfies all plugin dependencies and maximises each plugin's impact on the user's game.

LOOT also provides some load order error checking, including checks for requirements, incompatibilities and cyclic dependencies. In addition, it provides a large number of plugin-specific usage notes, bug warnings and Bash Tag suggestions.

libloot provides access to LOOT's metadata and sorting functionality, and the LOOT application is built using it.

## Downloads

Releases are hosted on [GitHub](https://github.com/loot/libloot/releases).


Snapshot builds are available as artifacts from [GitHub Actions runs](https://github.com/loot/libloot/actions), though they are only kept for 90 days and can only be downloaded when logged into a GitHub account. To mitigate these restrictions, snapshot build artifacts include a GPG signature that can be verified using the public key hosted [here](https://loot.github.io/.well-known/openpgpkey/hu/mj86by43a9hz8y8rbddtx54n3bwuuucg), which means it's possible to re-upload the artifacts elsewhere and still prove their authenticity.

The snapshot build artifacts are named like so:

```
libloot-<last tag>-<revisions since tag>-g<short revision ID>_<branch>-<platform>.<file extension>
```

## Building libloot

Refer to `.github/workflows/release.yml` for the build process.

### Linux

The build process assumes that you have already cloned the libloot repository,
that the current working directory is its root, and that the following
applications are already installed:

- `cmake`
- `curl`
- `git`
- `pip3` (and therefore Python 3)
- `cargo` and the rest of the Rust toolchain (e.g. via
  [rustup](https://rustup.rs/))
- `wget`

The list above may be incomplete.

### CMake Variables

libloot uses the following CMake variables to set build parameters:

Parameter | Values | Default |Description
----------|--------|---------|-----------
`BUILD_SHARED_LIBS` | `ON`, `OFF` | `ON` | Whether or not to build a shared libloot binary.
`LIBLOOT_BUILD_TESTS` | `ON`, `OFF` | `ON` | Whether or not to build libloot's tests.
`LIBLOOT_INSTALL_DOCS` | `ON`, `OFF` | `ON` | Whether or not to install libloot's docs (which need to be built separately).
`RUN_CLANG_TIDY` | `ON`, `OFF` | `OFF` | Whether or not to run clang-tidy during build. Has no effect when using CMake's MSVC generator.
`ESPLUGIN_URL` | A URL | A GitHub release archive URL | The URL to get a source code archive from. This can be used to supply a local path if the archive has already been downloaded (e.g. for offline builds).
`LIBLOADORDER_URL` | A URL | A GitHub release archive URL | The URL to get a source code archive from. This can be used to supply a local path if the archive has already been downloaded (e.g. for offline builds).
`LOOT_CONDITION_INTERPRETER_URL` | A URL | A GitHub release archive URL | The URL to get a source code archive from. This can be used to supply a local path if the archive has already been downloaded (e.g. for offline builds).
`FETCHCONTENT_SOURCE_DIR_YAML-CPP` | A path | Unset | The path to an existing yaml-cpp source folder to build yaml-cpp from. Note that libloot relies on [a fork of yaml-cpp](https://github.com/loot/yaml-cpp) to support YAML merge keys in metadata files. If unset, CMake will download the source from GitHub when the libloot build is configured.

You may also need to set `CMAKE_PREFIX_PATH` if CMake cannot find Boost.

## Building The Documentation

Install [Doxygen](https://www.doxygen.nl/), Python and [uv](https://docs.astral.sh/uv/getting-started/installation/) and make sure they're accessible from your `PATH`, then run:

```
cd docs
uv run -- sphinx-build -b html . ../build/docs/html
```
