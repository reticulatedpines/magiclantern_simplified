#!/usr/bin/env sh
# Development
ML_PKGS=("flex" "bison" "libgmp3-dev" "libmpfr-dev" "libncurses5-dev" "libmpc-dev" "autoconf" "texinfo" "build-essential" "git-core" "mercurial" "zlibc" "zlib1g-dev" "gcc-arm-none-eabi" ) # "gdb-multiarch" ) #this gdb is broken
# Documentation
ML_PKGS+=("python2-matplotlib" "python2-matplotlib-data" "python2-matplotlib-doc" "python2-matplotlib-dbg" "texlive-luatex" "python2-docutils" "texlive-latex-base" "texlive-latex-recommended" "texlive-fonts-extra" "dvi2ps" "texlive-science" "texlive-science-doc" "pandoc" "texlive-latex-extra" "ruby-full" "poppler-utils" "imagemagick" )

sudo apt-get install ${ML_PKGS[@]}
