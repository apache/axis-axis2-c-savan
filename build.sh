#!/bin/bash
./autogen.sh
./configure --prefix=${AXIS2C_HOME} --enable-static=no --with-axis2_util=${AXIS2C_HOME}/include/axis2-1.0 --with-axiom=${AXIS2C_HOME}/include/axis2-1.0
make
