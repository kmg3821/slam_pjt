const getEnv = function(){
	const promise = axios.get('/back')
	const data = promise.then((res)=>{
		return {'ip':res.data['ip'], 'port':res.data['port']};
	});
	return data;
}

const getImage = async function(backend){
	const imagePlaceholder = document.querySelector(".camera");
	let ok = false;
	let imageUrl = null;
	await axios.get(backend + "/image/recent")
	.then((res)=>{
		if(res.status === 200){
			imageUrl = res.data['image'];
			imagePlaceholder.src = backend + '/' + imageUrl;
		}
	})
	.catch((error)=>{
		ok = false;
	});
	if(ok){
		const image = await axios.get(backend + imageUrl);
		imagePlaceholder.style.backgroundImage = `url('${image}')`;
	}
	else{
		imagePlaceholder.style.backgroundImage = `placeholder.png`;
	}
	setTimeout(getImage, "100", backend);
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
	getImage(backend);
}

main();
