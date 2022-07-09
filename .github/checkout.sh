#!/bin/bash

desktop_branch=$(git rev-parse --abbrev-ref HEAD)

cd cryptonote || exit

if git checkout "$desktop_branch"; then
   git checkout "$desktop_branch"
else
    git checkout development
fi