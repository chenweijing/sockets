#!/bin/bash

echo "svn update .. "
cd /usr/work/message-queue 
svn update /usr/work/message-queue
cd /usr/src/sockets

echo "create protocol buffer files for c++."
protoc --proto_path=/usr/work/message-queue/protobuf/  --cpp_out=. /usr/work/message-queue/protobuf/machine_study.proto
echo "create buffer files ok."
