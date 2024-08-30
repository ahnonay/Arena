#!/bin/bash
sudo docker start arena_builder
sudo docker exec arena_builder rm -rf /root/src
sudo docker cp src/. arena_builder:/root/src
sudo docker exec arena_builder rm -rf /root/external
sudo docker cp external/. arena_builder:/root/external
sudo docker cp CMakeLists.txt arena_builder:/root/CMakeLists.txt
sudo docker exec --workdir /root arena_builder cmake -DCMAKE_BUILD_TYPE=Release -S . -B ./cmake-build-release
sudo docker exec --workdir /root arena_builder cmake --build ./cmake-build-release --target Arena -- -j 3
mkdir dockerbuild-result
chmod 777 dockerbuild-result
sudo docker cp arena_builder:/root/cmake-build-release/Arena dockerbuild-result/Arena
sudo docker stop arena_builder