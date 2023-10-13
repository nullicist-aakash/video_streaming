'use strict';
import {
    get_tcp_socket
} from './sockets';

const port = parseInt(process.argv[2]);
console.log(`Will open passive TCP connection on port ${port} for all interfaces.`);

let database = {};

const tcp = get_tcp_socket('0.0.0.0', port);

tcp.onNewConnection((IP, PORT) => {
    console.log(`New TCP connection from ${IP}:${PORT}.`);
});

tcp.onSocketError((err) => {
    console.log(`Error while opening TCP socket: ${err}`);
});

tcp.onMessage((IP, PORT, data) => {
    data = data.replace('\n', '');
    if (!database[data])
        database[data] = [];

    console.log(`Received data '${data}' from ${IP}:${PORT}.`);
    database[data].push([IP, PORT]);
    
    if (database[data].length == 2) {
        // Send peer details to both peers
        for (let i = 0; i < 2; ++i) {
            let [self_ip, self_port] = database[data][i];
            let [other_ip, other_port] = database[data][1 - i];

            tcp.send(self_ip, self_port, `${other_ip}:${other_port}`);
        }

        // Clean the entry for current device for future use
        delete database[data];
    }
});

tcp.onConnectionClose((IP, PORT) => {
    console.log(`Connection closed actively from ${IP}:${PORT}. I never close connections actively.`);

    for (let data in database) {
        let index = database[data].findIndex(value => value[0] == IP && value[1] == PORT);

        if (index == -1) continue;

        database[data].splice(index, 1);

        if (database[data].length == 0)
            delete database[data];
        break;
    }

    console.log(`Current database status: ${JSON.stringify(database)}`);
});