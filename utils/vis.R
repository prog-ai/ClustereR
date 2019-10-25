#!/usr/bin/env Rscript

library(ggplot2)
library(gganimate)

args <- commandArgs(trailingOnly = TRUE)
points <- args[1]
memory <- args[2]

df.points <- read.csv(points, sep = " ", header = FALSE)
colnames(df.points) <- c("x", "y")
df.points$label <- "points"

df.memory <- read.csv(memory, sep = " ", header = FALSE)
colnames(df.memory) <- c("iter", "x", "y")
df.memory$label <- "centroids"

animation <-
    ggplot(df.memory, aes(x = x, y = y, group = label, colour = label)) +
    geom_point(data = df.points, mapping = aes(x = x, y = y, group = label, colour = label), inherit.aes = FALSE) +
    geom_point() +
    labs(title = "Iteration: {frame_time}") +
    transition_time(iter) +
    ease_aes("linear")

anim_save("clusters.gif", animation = animation)
