#!/bin/sh

rm -rf venv
python3 -m venv venv
source ./venv/bin/activate
python3 -m pip install build
cd python
python3 -m build
cd ..
deactivate
rm -rf venv
