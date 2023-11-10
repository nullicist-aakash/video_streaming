FROM gcc

WORKDIR /usr/src/

COPY . .

RUN make

CMD  echo relay_ip $RELAY_SERVER_NAME >> config.txt && echo relay_port $RELAY_SERVER_PORT >> config.txt && echo 123 && make run