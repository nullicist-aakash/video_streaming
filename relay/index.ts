import {
    get_tcp_socket, get_udp_socket
} from './sockets';

const port = parseInt(process.argv[2]);
console.log(`Will open relay on port: ${port}`);

let database = []
const tcp = get_tcp_socket('localhost', port);

tcp.onNewConnection((IP, PORT) => {
    database.push([IP, PORT]);
    if (database.length != 2)
        return;

    for (let i = 0; i < database.length; i++) {
        const send_IP = database[i][0];
        const send_PORT = database[i][1];

        tcp.send(send_IP, send_PORT, `${database[1 - i][0]}:${database[1 - i][1]}`);
    }

    // clear the database
    database = [];
});

tcp.onMessage((IP, PORT, message) => {
    if (message !== `Break the connecti0n`)
        return;

    console.log(`TCP: ${IP}:${PORT} -> ${message}`);
    tcp.closeConnection(IP, PORT);
});