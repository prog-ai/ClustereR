# Installing Prerequisites for R Visualization Scripts

## Installing the [gifski](https://gif.ski "Gifski Homepage") GIF Renderer

It is recommended to install `gifski` via Rust's package manager `cargo`.
The `install-prerequisites.sh` script will install Rust as well as the necessary `gifski` package automatically only prompting during Rust installation for confirmation of the default install prefix.

## First Time R User

Before executing the `install-prerequisites.R` script, you need to specify a user writable installation path for R packages.
This can be done using the following steps:

```shell
# enter the interactive R console
$ R
# install your first R packages
> install.packages("devtools")
# prompt whether to install to a user local directory
# --> answer with yes
# prompt whether to create a user local install directory with a default name
# --> recommended to keep the default install directory and to answer with yes
# prompt to select a mirror for downloading the package
# --> choose any mirror of your liking
# try loading the package after installation
> library("devtools")
# troubleshooting, if the last step fails:
# - try removing previous package installations from the user local install directory and repeat the steps above
# - try removing previous package installations from the user local install directory, load the system default gcc compiler (in your shell: module purge && module load DEVELOP gcc) and repeat the steps above
```

## R Users, who already installed R packages before

If you already installed R packages before the `install-prerequisites.R` script should work out-of-the-box.
Using a custom install prefix for R packages is possible by configuring your global R environment or by specifying `lib=<prefix>` (for `install.packages`) and `lib.loc=<prefix>` (for `library`) when dealing with R packages.
To employ the second approach you need to go into the `install-prerequisites.R` script and adapt the function arguments to your needs.
Same applies for the provided `visualize-*.R` scripts.

