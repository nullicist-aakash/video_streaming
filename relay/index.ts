'use strict';
import {
    get_udp_socket
} from './sockets';

function searchStringInArray(str: string, strArray: string[]) {
    for (var j = 0; j < strArray.length; j++) 
        if (strArray[j].match(str))
            return true;
    return false;
}

const port = parseInt(process.argv[2]);
console.log(`Will open relay on UDP port: ${port}`);
const udp = get_udp_socket('0.0.0.0', port);

let database = [];

udp.onMessage((IP ,PORT, message) => {
    const to_add = `${IP}:${PORT}`;

    if (!searchStringInArray(to_add, database))
        database.push(to_add);
    else
        return;

    console.log(`Received message from ${database[-1]} with content: ${message}`);
    if (database.length != 2)
        return;

    for (let i = 0; i < database.length; i++) {
        const send_IP = database[i].split(':')[0];
        const send_PORT = parseInt(database[i].split(':')[1]);

        for (let j = 0; j < 5; ++j)
            udp.send(send_IP, send_PORT, `${database[1 - i]}`);
    }
    
    // clean database
    database.length = 0;
    database = [];
    udp.closeSocket();
});