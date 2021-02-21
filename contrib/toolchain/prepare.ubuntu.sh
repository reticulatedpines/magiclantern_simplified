#!/usr/bin/env bash
# Development
ML_PKGS=("flex" "bison" "libgmp3-dev" "libmpfr-dev" "libncurses5-dev" "libmpc-dev" "autoconf" "texinfo" "build-essential" "git-core" "mercurial" "zlibc" "zlib1g-dev" "gcc-arm-none-eabi" ) # "gdb-multiarch" ) #this gdb is broken
# Documentation
ML_PKGS+=("python-matplotlib" "python-matplotlib-data" "python-matplotlib-doc" "python-matplotlib-dbg" "texlive-luatex" "python-docutils" "texlive-latex-base" "texlive-latex-recommended" "texlive-fonts-extra" "dvi2ps" "texlive-science" "texlive-science-doc" "pandoc" "texlive-latex-extra" "ruby-full" "poppler-utils" "imagemagick" )

sudo apt-get install ${ML_PKGS[@]}
