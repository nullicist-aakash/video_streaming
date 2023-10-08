'use strict';
import {
    get_udp_socket
} from './sockets';

const get_peer_address = (port: number, relay_name: string, relay_port: number) => {
    console.log(`Will open current connection from port: ${port}`);

    const udp = get_udp_socket('0.0.0.0', port);
    
    for (let i = 0; i < 10; i++) {
        udp.send(relay_name, relay_port, 'Hello to relay!');
    }
    
    return new Promise((resolve, reject) => udp.onMessage((IP, PORT, message) => {
        udp.closeSocket();
        const peer_ip = message.split(':')[0];
        const peer_port = parseInt(message.split(':')[1]);
        console.log(`Peer IP: ${peer_ip}, Peer Port: ${peer_port}`);
        setTimeout(() => resolve({'IP': peer_ip, 'PORT': peer_port}), 1000);
    }));
};

const port = parseInt(process.argv[2]);
const relay_name = process.argv[3];
const relay_port = parseInt(process.argv[4]);
get_peer_address(port, relay_name, relay_port).then((peer_address) => {
    const udp = get_udp_socket('0.0.0.0', port);
    udp.onMessage((IP, PORT, message) => {
        console.log(`Message from ${IP}:${PORT}: ${message}`);
    });
    
    for (let i = 0; i < 2; i++) {
        udp.send(peer_address['IP'], peer_address['PORT'], 'Hello PEER!');
    }
});