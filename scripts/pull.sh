#!/bin/bash

cd "$(dirname ${BASH_SOURCE[0]})/.."
git pull
./scripts/update-submodules.sh

