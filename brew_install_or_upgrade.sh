#!/usr/bin/env bash

package=$1
verbose=true

brew update >/dev/null 2>&1
list_output=$(brew list | grep "$package")
outdated_output=$(brew outdated | grep "$package")

# enable error checking
set -e

if [[ -n "$list_output" ]]; then
    $verbose && echo "package $package is installed"
    if [[ -z "$outdated_output" ]]; then
        $verbose && echo "package $package is updated"
    else
        $verbose && echo "package $package is not updated. updating..."
        brew upgrade "$package"
    fi
else
    $verbose && echo "package $package is not installed. installing..."
    brew install "$package"
fi