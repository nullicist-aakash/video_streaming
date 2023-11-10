FROM gcc

WORKDIR /usr/src/

COPY . .

RUN make

CMD ./relay.out $RELAY_SERVER_PORT