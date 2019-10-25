#!/usr/bin/env zsh
# 2019-03-22 Christian Wassermann (cw585968)
# script to install prerequisites for the provided R visualization script

# get rust for gifski renderer
curl https://sh.rustup.rs -sSf | sh
# follow the installation instructions...
# activate the Rust environment in the current shell
source ${HOME}/.cargo/env

# change modules due to compilation error with icc
module purge && module load DEVELOP gcc
cargo install gifski

if [ ! -d "${HOME}/R" ]; then
  mkdir ${HOME}/R
fi
./utils/install-prerequisites.R


