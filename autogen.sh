#!/bin/bash
aclocal
autoconf
automake --add-missing --foreign
./configure
