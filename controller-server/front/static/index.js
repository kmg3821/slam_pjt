const FRAME = 20;
const imagePlaceholder = document.querySelector(".camera");
const fpsPlaceholder = document.querySelector(".fps");

class FPSManager{
	constructor() {
		this.capturedtime = []
	}
	capture(){
		this.capturedtime.push(Date.now())
	}
	getFPS(){
		const now = Date.now();
		const start = now - 1000;
		while(this.capturedtime[0] < start){
			this.capturedtime.shift();
		}
		return this.capturedtime.length;
	}
}

const getEnv = function(){
	const promise = axios.get('/back')
	const data = promise.then((res)=>{
		return {'ip':res.data['ip'], 'port':res.data['port']};
	});
	return data;
}

const getImage = async function(backend){
	let ok = false;
	let imageUrl = null;
	await axios.get(backend + "/image/recent")
	.then((res)=>{
		if(res.status === 200){
			imageUrl = res.data['image'];
			imagePlaceholder.src = backend + '/' + imageUrl;
			ok = true;
		}
		else if(res.status !== 304){
			ok = false;
		}
	})
	.catch((error)=>{
		ok = false;
	});
	return ok;
}

const setFPS = (fps) =>{
	fpsPlaceholder.textContent = fps;
}

const onFrame = async function(backend, fpsmanager){
	const ok = await getImage(backend);
	fpsmanager.capture();
	setFPS(fpsmanager.getFPS());
	setTimeout(onFrame, Math.floor(1000/FRAME), backend, fpsmanager);
}

const main = async function(){
	const env = await getEnv();
	console.log(env);
	const backend = `http://${env['ip']}:${env['port']}`;
	console.log("backend", backend);
	const topButton        = document.querySelector(".btn-top");
	const leftButton       = document.querySelector(".btn-left");
	const rightButton      = document.querySelector(".btn-right");
	const downButton       = document.querySelector(".btn-down");
	const eventList        = [
		[topButton, "top"],
		[downButton, "bottom"],
		[leftButton, "left"],
		[rightButton, "right"]
	];
	eventList.forEach((item)=>{
		item[0].addEventListener('pointerdown', ()=>{
			const body = {
				key: item[1],
				pushed: true,
			};
			axios.post(backend + "/event", body);
		});
	});

	eventList.forEach((item)=>{
		item[0].addEventListener('pointerup', ()=>{
			const body = {
				key: item[1],
				pushed: false,
			};
			axios.post(backend + "/event", body);
		});
	});
	const fpsmanager = new FPSManager();
	console.log(fpsmanager);
	onFrame(backend, fpsmanager);
}

main();
