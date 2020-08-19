#!/bin/sh

docker exec -it sheerproxy-centos /bin/bash -c "./configure && make clean && make; $1"
