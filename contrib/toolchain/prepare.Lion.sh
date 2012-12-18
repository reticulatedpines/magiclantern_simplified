#!/bin/bash
 ###################################################
# Install things needed by MagicLantern on OSX Lion #
#     Still missing something for docs building     #
 ###################################################
#Using instructions
if [ "$1" == h ] || [ "$1" == help ]; then
	echo "Usage: prepare.Lion.sh [option]"
	echo ""
	echo "If there is no option set it will perform the basic"
	echo "install for converting 422 and merging HDR"
	echo ""
	echo "Options:"
	echo "h      :shows this help (also help is a valid option)"
	echo "dev    :install all the dependencies for the arm"
	echo "        toolchain and starts the summon-arm script"
	echo "remove :uninstall all the dependencies previously installed"
	echo "        it will ask for confirmation before going on"
	exit 0
fi

#BASIC TOOL FOR HDR, 422 to JPG etc.
BREW_ML="
mercurial
imagemagick
mplayer"

PIP_ML="
numpy
PIL"
# ALLMOST ALL THING NEEDED FOR COMPILING ML, DOC AND EXTRAS
if [ "$1" == dev ] || [ "$1" == d ] || [ "$1" == remove ] || [ "$1" == r ];then
BREW_ML="
binutils
gmp
mpfr
libmpc
mercurial
wget
imagemagick
ufraw
mplayer
poppler"

PIP_ML="
numpy
PIL
matplotlib 
docutils"
fi

echo "*******************************************"
echo "*     Checking for Homebrew and pip...    *"
echo "*******************************************"
if [ $(which brew) ]; then
	echo "Homebrew already installed"
else
	echo "Installing Homebrew..."
	/usr/bin/ruby -e "$(/usr/bin/curl -fsSL https://raw.github.com/mxcl/homebrew/master/Library/Contributions/install_homebrew.rb)"
	echo "Done"
    brew doctor
fi
echo "Updating Homebrew..."
brew update
echo "Done"
if [ $(which pip) ]; then
	echo "pip already installed"
else
	echo "Installing pip..."
	sudo easy_install pip
	echo "Done"
fi
	echo "Updating pip..."
	sudo pip install -U pip
	echo "Done"
echo


# Uninstalling
if [ "$1" == r ] || [ "$1" == remove ];then
	 echo "Are you sure to remove all thing installed? Y or N"
	 read uninstall
	 if [ "$uninstall" == Y ] || [ "$uninstall" == y ]; then
		for f in $BREW_ML; do
			echo "brew uninstall $f"
			brew uninstall "$f"
		done
		for f in $PIP_ML; do
			echo "pip uninstall $f"
			pip uninstall "$f"
		done
	fi
	exit 1
fi
echo "*******************************************"
echo "*          Checking dependencies:         *"
echo "*******************************************"
	#if [ ! $(brew list | grep apple-gcc42) ]; then
	#	if [ ! $(brew search apple-gcc42) ]; then
	#	brew tap homebrew/dupes
	#	fi
	#	echo "apple-gcc42 Installation (brew)"
	#	brew install apple-gcc42
	#else
	#echo "apple-gcc42 already installed (brew)"
	#fi
for f in $BREW_ML; do
	if [ ! $(brew list | grep "$f") ]; then
	echo "$f Installation (brew)"
	brew install "$f"
	else
	echo "$f already installed (brew)"
	fi
done	
for f in $PIP_ML; do
	if [ ! $(pip freeze | grep "$f") ]; then
	echo "$f Installation (pip)"
	sudo pip install "$f"
	else
	echo "$f already installed (pip)"
	fi
done
if [ ! $(which enfuse) ]; then
#	at this stage we may not have already installed wget, so here is better have curl -OL
	curl -OL http://downloads.sourceforge.net/project/enblend/enblend-enfuse/enblend-enfuse-4.0/enblend-enfuse-4.0-mac.tar.gz
#	wget http://downloads.sourceforge.net/project/enblend/enblend-enfuse/enblend-enfuse-4.0/enblend-enfuse-4.0-mac.tar.gz
	echo "enfuse installation (tar)"
	tar xvfz enblend-enfuse-4.0-mac.tar.gz
	cp enblend-enfuse-4.0-mac/enfuse /usr/bin/
	cp enblend-enfuse-4.0-mac/enblend /usr/bin/
	rm -dR enblend-enfuse-4.0-mac
else
	echo "enfuse already installed"
fi
# installing develop dipendencies
if [ "$1" == dev ] || [ "$1" == d ]; then
	if [ ! $(which libusb-config) ]; then
		echo "Downloading libusb.pkg"
		wget http://www.ellert.se/PKGS/libusb-2011-10-29/10.7/libusb.pkg.tar.gz
		tar xvfz libusb.pkg.tar.gz
		echo "libusb.pkg  Installation"
		sudo installer -pkg libusb.pkg -target /
	else
		echo "libusb already installed"
	fi
	if [ ! $(which pandoc) ] ; then
		echo "pandoc Installation (pkg)"
		wget http://pandoc.googlecode.com/files/pandoc-1.9.4.1.dmg
		hdiutil attach pandoc-1.9.4.1.dmg
		sudo installer -pkg /Volumes/pandoc\ 1.9.4.1/pandoc-1.9.4.1.pkg -target /
	else
		echo "pandoc already installed"
	fi
	if [ ! $(which tex) ]; then
		echo
		echo "To be able to compile docs"
		echo "YOU NEED TO INSTALL MacTeX"
		echo "http://www.tug.org/mactex/"
		echo "Searching Installer..."
		echo
		mactex=$(mdfind MacTeX-2011.mpkg | grep MacTeX-2011.mpkg)
		if [ ! $mactex ]; then
			echo "Downloading MacTeX"
			wget http://mirror.ctan.org/systems/mac/mactex/MacTeX.mpkg.zip
			unzip MacTeX.mpkg.zip
			open MacTeX-2011.mpkg
		else
			open $mactex
		fi
	else
		echo "tex already installed (dmg)"
	fi
fi