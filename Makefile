SHELL := /bin/bash
.DEFAULT_GOAL := host
.PHONY: host test deps ps5 xenia-host package
host:
	cmake -S . -B build/host -G Ninja -DCMAKE_CXX_COMPILER=clang++-19
	cmake --build build/host -j 4
test: host
	ctest --test-dir build/host --output-on-failure
deps:
	bash tools/bootstrap.sh
ps5:
	bash tools/build-ps5.sh
xenia-host: host
	cmake -S tools/xenia-core -B build/xenia-host -G Ninja -DCMAKE_C_COMPILER=clang-19 -DCMAKE_CXX_COMPILER=clang++-19
	cmake --build build/xenia-host -j 4
	timeout 60 build/xenia-host/xenia-ppc-probe build/host/logs
package:
	python3 tools/source-package.py
	python3 tools/package.py
