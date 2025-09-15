# What is this?
This directory defines a "Dev Containers" environment that provides all necessary tools to compile "Magic Lantern" while keeping the host system clean of these dependencies.

# Prerequisites
* Install the "Dev Containers" extension for Visual Studio Code (`ms-vscode-remote.remote-containers`)
* Make sure that Docker is installed and running

# Usage
Open the root directory of the Git repository in Visual Studio Code. A dialog with the option "Reopen in Container" should pop up. Click it to build and enter the container environment. In case the dialog is not present select "Dev Containers: Reopen in Container" from the command palette.

In the container you can now open a new terminal and build "Magic Lantern as follows" with the following commands:
```
cd platform
# Adjust to your number of cores
make -j 12
```

# Notes
The user in the container is called `build` and has a user id and group id of 1000. These ids should in most cases coincide with the ids of the user of a single user Linux system. If that's the case then all files which are created during the build should have be owned by the user on the host system.

To save some space Git is not installed in the dev container environment as it is assumed that it is present on the host system anyway.