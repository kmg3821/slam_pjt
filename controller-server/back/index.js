const mqtt      = require('mqtt');
const fs        = require('fs');
const AsyncLock = require('async-lock');
const express   = require('express');
const cors      = require('cors');
const morgan    = require('morgan');
const config    = require('../../config/config.js');
const env       = new config('../../config/config.txt');

const backend_port = Number(env.get('BACKEND_PORT'));
var imageSrc = null;
var imageList = []

const lock = new AsyncLock();
const app = express();
const client = mqtt.connect({
	host: env.get('MQTT_BROKER_IP'),
	port: Number(env.get('MQTT_BROKER_PORT')),
	protocol: 'mqtt',
});

class MQTTMessageHandler{
	constructor(){
		this.handler = {};
	}
	addHandler(topic, fn){
		this.handler.topic = fn;
	}
	handle(topic, message){
		if(this.handler.topic === undefined){
			return;
		}
		return this.handler.topic(message);
	};
}

const main = function(){

	app.use(cors());
	app.use(express.json());
	app.use('/image', express.static('image'))

    app.get('/image/recent', (req,res)=>GetRecentImage(req, res));
	app.post('/event', (req,res)=>PostEvent(req, res));
	
	const messageHandler = new MQTTMessageHandler();
	messageHandler.addHandler('img', (message)=>messageImgHandler(message));

	client.on('message', (topic, message) => messageHandler.handle(topic, message));
	client.on('error', (error)=>console.log(`Cannot connect to MQTT: ${error}`));
	client.on('connect', ()=>mqttConnectHandler(client));
	app.listen(backend_port, ()=>console.log(`Webserver backend is ready on ${backend_port}`));
	
}

main();

const GetRecentImage = (req, res)=>{
	status_code = 100;
	msg = {"status":"continue..."};
	lock.acquire('file-io', (done)=>{
		if (imageSrc === null){
			status_code = 204;
			msg = {"status": "image not taken yet"};
		}
		else{
			status_code = 200;
			msg = {"status": "OK", "image":imageSrc};
		}
		done();
	});
	return res.status(status_code).json(msg);
};

const PostEvent = (req, res)=>{
	client.publish(`rc/${req.body.key}`, (req.body.pushed?"1":"0"));
	return res.status(200).json({"status":"received"});	
};

const mqttConnectHandler = (client) => {
	console.log("MQTT Client connected to port 1883");
	client.subscribe('img', {qos:0});
	const imageDir = './image'
	if(!fs.existsSync(imageDir)){
	    fs.mkdirSync(imageDir);
	}
	fs.readdirSync(imageDir, (err, filelist)=>{
		filelist.forEach((file)=>{
			if(fs.lstatSync(file).isFile()){
				fs.unlink(file, (err)=>{
					console.error(err);
				});
			}
		});
	});
};

const messageImgHandler = (message)=>{
	const thisImageSrc = `image/received-${message.readBigUint64BE(4)}.jpg`;
	let imageToUnlink = null;
	lock.acquire('file-io', (done)=>{
		imageSrc = thisImageSrc;
		imageList.push(imageSrc);
		if(imageList.length >= 10){
			imageToUnlink = imageList[0];
			imageList.shift();
		}
		done();
	});
	if(imageToUnlink){
		fs.unlink(imageToUnlink, (err)=>{
			if(err){
				console.error(err);
				return;
			}
		});
	};
	fs.writeFile(thisImageSrc, message.slice(12), (err)=>{
		if(err){
			console.log(err);
		}
	});
}

