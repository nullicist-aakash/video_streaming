'use strict';
import * as dgram from 'node:dgram';
import { EventEmitter } from 'events';
import * as net from 'net';

export function get_udp_socket(self_name: string, port: number) {
  // create socket
  const udp_socket = dgram.createSocket('udp4');

  // event listener in case of error
  udp_socket.on("error", error => {
    udp_socket.close();
    console.log(`UDP Server Error: ${error.message}`);
    throw error;
  });

  // start udp server
  udp_socket.bind(port, self_name, () => {
    const address = udp_socket.address();
    console.log(`UDP Process started on ${address.address}:${address.port}`);
  });

  const udp_emitter = new EventEmitter();
  const onMessage = (func: (IP: string, PORT: number, msg: string) => void) =>
    udp_socket.on('message', (msg, rinfo) => func(rinfo.address, rinfo.port, msg.toString('utf8')));
  
  const onSend = (func: (IP: string, PORT: number, msg: string) => void) => udp_emitter.on('on_msg_send', func);

  const onError = (func: (error: Error) => void) =>
    udp_socket.on("error", error => func(error));

  const send = (address: string, port: number, msg: string) => {
    udp_socket.send(msg, port, address);
    udp_emitter.emit('on_msg_send', address, port, msg);
  };

  const closeSocket = () => udp_socket.close();

  return { onMessage, onSend, onError, send, closeSocket };
}

export function get_tcp_socket(self_name: string, port: number) {
  // create socket
  const tcp_socket = net.createServer();
  const connections : { [key: string]: net.Socket } = {};

  // event listener in case of error
  tcp_socket.on("error", error => {
    tcp_socket.close();
    console.log(`TCP Server Error: ${error.message}`);
    throw error;
  });

  // start tcp server
  tcp_socket.listen(port, self_name, () => {
    const address = tcp_socket.address() as net.AddressInfo;
    console.log(`TCP Process started on ${address.address}:${address.port}`);
  });

  const verifyAddress = (IP: string, PORT: number) : string => {
    const name = `${IP}:${PORT}`;
    if (!(name in connections)){
      console.log(`Accessing a TCP connection ${name} which is not associated with current socket? Crashing the server!`);
      process.exit();
    }
    return name;
  }

  // Define Callbacks
  const tcp_emitter = new EventEmitter();

  const onMessage = (func: (IP: string, PORT: number, data: string) => void) => tcp_emitter.on('on_msg', func);
  const onSend = (func: (IP: string, PORT: number, data: string) => void) => tcp_emitter.on('on_msg_send', func);

  const send = (IP: string, PORT: number, data: string) : void => {
    const name = verifyAddress(IP, PORT);
    const socket = connections[name];
    socket.write(data);
    tcp_emitter.emit('on_msg_send', IP, PORT, data);
  };

  const onNewConnection = (func: (IP: string, PORT: number) => void) => tcp_emitter.on('on_new_conn', func);
  const onConnectionClose = (func: (IP: string, PORT: number) => void) => tcp_emitter.on('on_conn_close', func);

  const closeConnection = (IP: string, PORT: number) => {
    const name = verifyAddress(IP, PORT);
    connections[name].destroy();
    delete connections[name];
  };

  const onConnectionError = (func: (IP: string, PORT: number, error: Error) => void) => tcp_emitter.on('on_conn_err', func);

  const onSocketError = (func: (error: Error) => void) => tcp_socket.on('error', err => func(err));

  const closeSocket = () => tcp_socket.close();

  // Connect events with callbacks
  tcp_socket.on('connection', (sock: net.Socket) => {
    const IP = sock.remoteAddress;
    const PORT = sock.remotePort;
    const name = `${IP}:${PORT}`;

    connections[name] = sock;
    tcp_emitter.emit('on_new_conn', IP, PORT);

    sock.on('data', (data: Buffer) => tcp_emitter.emit('on_msg', IP, PORT, data.toString()));
    sock.on('close', _ => tcp_emitter.emit('on_conn_close', IP, PORT));
    sock.on('error', (err: Error) => tcp_emitter.emit('on_conn_err', IP, PORT, err));
  });


  return {
    onMessage, onSend, send,
    onNewConnection, onConnectionClose, closeConnection,
    onConnectionError,
    onSocketError, closeSocket
  };
}