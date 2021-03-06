# usrsctp
[![Coverity Scan Build Status](https://scan.coverity.com/projects/13430/badge.svg)](https://scan.coverity.com/projects/usrsctp)

This is a userland SCTP stack supporting FreeBSD, Linux, Mac OS X and Windows.

See [manual](Manual.md) for more information.

The status of continuous integration testing is available from [grid](http://212.201.121.110:18010/grid) and [waterfall](http://212.201.121.110:18010/waterfall).
If you are only interested in a single branch, just append `?branch=BRANCHNAME` to the URL, for example [waterfall](http://212.201.121.110:18010/waterfall?branch=master).

## how to use it

1. clone this projects
    ```
    git clone https://github.com/gidcs/usrsctp
    cd usrsctp
    ```

2. use autotools or cmake3 to build this projects

    ```
    ./bootstrap
    ./configure
    make
    ```

    ```
    mkdir build
    cd build
    cmake3 ..
    make
    ```
