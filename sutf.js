const WebSocket = require('ws');

// Create a new WebSocket connection
const socket = new WebSocket('ws://192.168.26.62/ws');

// Connection opened
socket.on('open', function open() {
    console.log('Connected to WebSocket server');
    // Send a message to the server
    socket.send(JSON.stringify({ type: 'greeting', message: 'Hello Server!' }));
});

// Listen for messages
socket.on('message', function incoming(data) {
    const parsedData = JSON.parse(data);
    console.log('Message from server ', parsedData);
    // Handle the received data
    if (parsedData.type === 'response') {
        console.log('Response from server:', parsedData.message);
    }
});

// Listen for errors
socket.on('error', function error(event) {
    console.error('WebSocket error: ', event);
});

// Connection closed
socket.on('close', function close(event) {
    console.log('WebSocket connection closed: ', event);
});
