const mqtt = require('mqtt');
const fs = require('fs');
const AsyncLock = require('async-lock');
const express = require('express');
const cors = require('cors');
const morgan = require('morgan');

const app = express();
app.use(cors());
app.use('/image', express.static('image'));
app.use(morgan('dev'));

const backend_port = 8081;

const lock = new AsyncLock();
var imageSrc = null;
const client = mqtt.connect({
	host: '127.0.0.1',
	port: 1883,
	protocol: 'mqtt',
});

app.get('/image/recent', (req, res)=>{
	status_code = 100
	json = {"status":"continue..."};
	lock.acquire('file-io', ()=>{
		if (imageSrc === null){
			status_code = 404;
			json = {"status": "image not taken yet"};
		}
		else{
			status_code = 200;
			json = {"status": "OK", "image":imageSrc}
		}
	})
	return res.status(status_code).json(json);
});

app.listen(backend_port, ()=>{
	console.log(`Webserver backend is ready on port ${backend_port}`);
})


client.on("connect", () => {
	console.log("MQTT Client connected to port 1883");
	client.subscribe('img', {qos:2});
});

client.on("error", (error)=> {
	console.log("Cannot connect to MQTT:" + error);
})

client.on('message', (topic, message, packet)=>{
	if (topic === 'img'){
		console.log(`Message received : topic=[${topic}]`);
		console.log(`image size is : ${message.readUInt32BE(0)}`);
		console.log(`timestamp  is : ${message.readBigUInt64BE(4)}`);
		lock.acquire('file-io', ()=>{
			imageSrc = `image/received-${message.readBigUInt64BE(4)}.png`;
			fs.writeFile(imageSrc, message.slice(12), (err)=>{
				if(err) {
					console.log(err);
				}
			});
		});
	}
});

