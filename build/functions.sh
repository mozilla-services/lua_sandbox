#!/bin/sh

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Author: Mathieu Parent <math.parent@gmail.com>

# =================================================================
# Show usage
usage() {
    echo "Build, test and make packages (optionally using Docker)" >&2
    echo "  $0 build" >&2
    echo "" >&2
    echo "Options are passed using environment variables:" >&2
    echo "  - DOCKER_IMAGE: Run the build in a container (default: <none>)" >&2
    echo "  - CPACK_GENERATOR: DEB, RPM, TGZ, ... (default guessed)" >&2
    echo "  - CMAKE_URL: If set, download cmake from it (default: <none>, auto also supported)" >&2
    echo "  - CMAKE_SHA256: Used to checksum CMAKE_URL (default: <none>)" >&2
    echo "  - CC: C compiler (default: gcc)" >&2
    echo "  - CXX: C++ compiler (default: g++)" >&2
}


# =================================================================
# Run command within container
docker_run() {
    DOCKER_IMAGE="${DOCKER_IMAGE:-debian:jessie}"
    DOCKER_VOLUME="$(dirname "$PWD")"
    (   set -x
        docker pull "$DOCKER_IMAGE"
        ret=0
        docker run \
            --volume "${DOCKER_VOLUME}:${DOCKER_VOLUME}" \
            --workdir "$PWD" \
            --rm=true --tty=true \
            --env "CPACK_GENERATOR=${CPACK_GENERATOR}" \
            --env "CMAKE_URL=${CMAKE_URL}" \
            --env "CMAKE_SHA256=${CMAKE_SHA256}" \
            --env "CC=${CC}" \
            --env "CXX=${CXX}" \
            --env "DISTRO=${DOCKER_IMAGE}" \
            "$DOCKER_IMAGE" \
            $@
    )
}

# =================================================================
# Guess environment variables
setup_env() {
    if [ "$(id -u)" = 0 ]; then
        as_root=""
    else
        as_root="sudo"
    fi
    if [ -z "${CPACK_GENERATOR}" ]; then
        if [ -x "`command -v apt-get 2>/dev/null`" ]; then
            CPACK_GENERATOR=DEB
        elif [ -x "`command -v rpm 2>/dev/null`" ]; then
            CPACK_GENERATOR=RPM
        else
            CPACK_GENERATOR=TGZ
        fi
    fi
    CC="${CC:-gcc}"
    CXX="${CXX:-g++}"
}

# =================================================================
# Install cmake from URL
install_cmake() {
    if echo "$PATH" | grep -qF "/cmake-dist/bin:" ; then
        # Already done
        return
    fi
    if [ "$CMAKE_URL" = "auto" ]; then
        CMAKE_URL="https://cmake.org/files/v3.7/cmake-3.7.1-Linux-x86_64.tar.gz"
        CMAKE_SHA256=7b4b7a1d9f314f45722899c0521c261e4bfab4a6b532609e37fef391da6bade2
    elif [ -z "$CMAKE_URL" ]; then
        echo "Missing env CMAKE_URL"
        exit 1
    elif [ -z "$CMAKE_SHA256" ]; then
        echo "Missing env CMAKE_SHA256"
        exit 1
    fi
    (   set -x
        wget -O cmake.tar.gz "${CMAKE_URL}"
        echo "${CMAKE_SHA256}  cmake.tar.gz" > cmake-SHA256.txt
        sha256sum --check cmake-SHA256.txt || {
            echo 'Checksum error'
            exit 1
        }
        mkdir cmake-dist
        cd cmake-dist
        tar xzf "../cmake.tar.gz" --strip-components=1
    )
    PATH="$PWD/cmake-dist/bin:${PATH}"
    echo "+PATH=${PATH}"
}

# =================================================================
# Install packages from distribution
# The following substitutions are done:
# - cmake is replaced by downloaded, if CMAKE_URL is set
# - rpm-build is ignored on DEB distributions
# - ca-certificates is ignored on RPM distributions
install_packages() {
    local packages
    packages="$@"
    do_install_cmake=no
    if [ -n "$CMAKE_URL" ] && echo " $packages " | grep -qF " cmake "; then
        packages="$(echo " $packages " | sed "s/ cmake / wget ca-certificates /")"
        do_install_cmake=yes
    fi
    packages="$(echo " $packages " | sed -e "s/ c-compiler / $CC /" -e "s/ c++-compiler / $CXX /")"
    if [ "$CPACK_GENERATOR" = "DEB" ]; then
        packages="$(echo "$packages" | sed -e "s/ rpm-build / /" -e "s/ clang++ / clang /")"
        (   set -x;
            $as_root apt-get update
            $as_root apt-get install -y $packages
        )
    elif [ "$CPACK_GENERATOR" = "RPM" ]; then
        packages="$(echo "$packages" | sed -e "s/ ca-certificates / /" -e "s/ g++ / gcc-c++ /" -e "s/ clang++ / clang /")"
        (   set -x;
            $as_root yum install -y $packages
        )
    else
        echo "Unhandled CPACK_GENERATOR: $CPACK_GENERATOR" >&2
        exit 1
    fi
    if [ "$do_install_cmake" = "yes" ]; then
        install_cmake
    fi
}

# =================================================================
# Install all packages from directory
install_packages_from_dir() {
    local dir
    dir="$1"
    if [ ! -d "$dir" ]; then
        echo "Not a directory: $dir"
        exit 1
    fi
    if [ "$CPACK_GENERATOR" = "DEB" ]; then
        (set -x; $as_root dpkg -i "$dir"/*.deb)
    elif [ "$CPACK_GENERATOR" = "RPM" ]; then
        (set -x; $as_root rpm -i "$dir"/*.rpm)
    else
        echo "Unhandled CPACK_GENERATOR: $CPACK_GENERATOR" >&2
        exit 1
    fi
}

# =================================================================
# Build
build() {
    (   set -x

        rm -rf ./release
        # From README.md:
        mkdir release
        cd release

        cmake -DCMAKE_BUILD_TYPE=release ..
        make

        ctest -V
        cpack -G "${CPACK_GENERATOR}"
    )
}

# =================================================================
# Main
main() {
    if [ -n "$DOCKER_IMAGE" ]; then
        docker_run "$0" build
    else
        setup_env
        install_packages c-compiler cmake make rpm-build
        if [ "$(basename "$PWD")" != "lua_sandbox" ]; then
            local old_dir
            local lsb_dir
            old_dir="$PWD"
            lsb_dir="$(dirname "$old_dir")/lua_sandbox"
            echo "+cd $lsb_dir"
            cd "$lsb_dir"
            build
            install_packages_from_dir ./release
            echo "+cd $old_dir"
            cd "$old_dir"
        fi
        if [ -n "$build_function" ]; then
            "$build_function"
        else
            build
        fi
    fi
}
