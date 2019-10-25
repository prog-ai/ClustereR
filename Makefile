EXECUTABLE := kmeans.exe
SRC := kmeans.cpp

# INPUT_DIR needs to be set according to your local file placement
INPUT_DIR := input

CXX ?= icpc
FLAGS_FAST = -O3 -ip -xhost -fp-model fast=2 -std=c++11
FLAGS_DEBUG = -O0 -g -std=c++11 -Wall -Wextra

build: help

help:
	@echo
	@echo '  Usage:'
	@echo '    make <target>'
	@echo
	@echo '  Targets:'
	@echo '    debug         Build for debugging'
	@echo '    release       Build for performance'
	@echo '    run-small     Run kmeans for the small data set'
	@echo '    run-large     Run kmeans for the large data set'
	@echo '    vis-small     Generate cluster.gif for the small data set'
	@echo '    vis-large     Generate cluster.gif for the large data set'
	@echo '    install-vis   Install requirements for the R visualization script'
	@echo '    clean         Clean-up of generated files'
	@echo

debug:
	${CXX} ${FLAGS_DEBUG} ${SRC} -o ${EXECUTABLE}

release:
	${CXX} ${FLAGS_FAST} ${SRC} -o ${EXECUTABLE}

run-small: release
	./${EXECUTABLE} ./input/small.in 1000 5 20

run-large: release
	./${EXECUTABLE} ./input/large.in 1000000 5000 50

vis-small:
	R_LIBS_USER=~/R ./utils/vis.R ./input/small.in result.out

vis-large:
	R_LIBS_USER=~/R ./utils/vis.R ./input/large.in result.out

install-vis:
	./utils/install-prerequisites.sh

clean:
	${RM} ${EXECUTABLE} *.out *.gif

.PHONY: clean build debug release run-small run-large vis-small vis-large
