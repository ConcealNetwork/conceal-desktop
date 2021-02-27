#!/usr/bin/env bash

package=$1
pkg_installed=false
pkg_updated=false
verbose=true

brew update >/dev/null 2>&1
list_output=`brew list | grep $package`
outdated_output=`brew outdated | grep $package`

# enable error checking
set -e

if [[ ! -z "$list_output" ]]; then
    pkg_installed=true
    $verbose && echo "package $package is installed"
    if [[ -z "$outdated_output" ]]; then
        pkg_updated=true
        $verbose && echo "package $package is updated"
    else
        $verbose && echo "package $package is not updated. updating..."
        brew upgrade $package
    fi
else
    $verbose && echo "package $package is not installed. installing..."
    brew install $package
fi