import * as net from 'net';


const port = parseInt(process.argv[2]);
const relay_ip = process.argv[3];
const relay_port = parseInt(process.argv[4]);
console.log(`Will open peer on port: ${port}`);

let server_ip = null;
let server_port = null;

let client = new net.Socket();