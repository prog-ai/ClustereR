#!/usr/bin/env Rscript
# 2019-03-22 Christian Wassermann (cw585968)
# script to install package prerequisites for the provided R visualization script

install.packages(
    c(
        "ggplot2",
        "gganimate",
        "png",
        "gifski"
    ),
    repos = "http://cran.r-project.org",
    lib="~/R"
)
