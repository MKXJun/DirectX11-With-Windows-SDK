@echo off

cmake -S . -B build
cd build
del /S/Q *.cso
cmake --build . --config Release -- -m
cd ..
