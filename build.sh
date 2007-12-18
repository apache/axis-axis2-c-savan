#!/bin/bash
./autogen.sh
./configure --prefix=$AXIS2C_HOME --enable-static=no --with-axis2=${AXIS2C_HOME}/include/axis2-1.2 --enable-filtering=no
make -j30
make install

cd samples
sh autogen.sh
./configure --prefix=${AXIS2C_HOME} --with-axis2=${AXIS2C_HOME}/include/axis2-1.2 --with-savan=${AXIS2C_HOME}/include/savan-0.90
make -j10
make install
cd ..
