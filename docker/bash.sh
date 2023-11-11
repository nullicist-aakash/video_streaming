#!/bin/sh
set -x

cd ../peer
make
cd ../relay
make
cd ../docker

docker stop relay || true
docker stop peer1 || true
docker stop peer2 || true
docker container prune -f || true
docker network rm video_streaming || true

docker network create -d bridge video_streaming

docker run --name relay -itd --network=video_streaming gcc-updated bash
docker cp relay.out relay:/usr/src/relay.out

docker run --name peer1 --cap-add=NET_ADMIN -itd --network=video_streaming --mount source=tcpdump,target=/usr/src/ gcc-updated bash
docker run --name peer2 --cap-add=NET_ADMIN -itd --network=video_streaming --mount source=tcpdump,target=/usr/src/ gcc-updated bash

Shared with peer2 automatically
docker cp peer.out peer1:/usr/src/peer.out
docker cp config.txt peer1:/usr/src/config.txt

docker exec peer1 bash -c 'tcpdump -Xi any -w peer1.pcap &' &
docker exec peer2 bash -c 'tcpdump -Xi any -w peer2.pcap &' &