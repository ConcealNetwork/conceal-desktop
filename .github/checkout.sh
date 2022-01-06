#!/bin/bash

desktop_branch=$(git rev-parse --abbrev-ref HEAD)

cd cryptonote || exit

if git show-ref --verify --quiet refs/heads/"$desktop_branch"; then
   git checkout "$desktop_branch"
else
    git checkout development
fi