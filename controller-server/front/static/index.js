const modelServer      = env['modelServer']
const modelPort        = 8081

const backend          = "http://" + modelServer + ":" + modelPort;

const imagePlaceholder = document.querySelector(".camera");
const topButton        = document.querySelector(".btn-top");
const leftButton       = document.querySelector(".btn-left");
const rightButton      = document.querySelector(".btn-right");
const downButton       = document.querySelector(".btn-down");


const getImage = async function(){
	let ok = false;
	let imageUrl = null;
	await axios.get(backend + "/image/recent")
	.then((res)=>{
		if(res.status === 200){
			console.log(res.data);
			console.log(res.data['image']);
			imageUrl = res.data['image'];
			imagePlaceholder.src = backend + '/' + imageUrl;
			console.log(backend + imageUrl);
		}
	})
	.catch((error)=>{
		ok = false;
	});
	if(ok){
		const image    = axios.get(backend + imageUrl);
		imagePlaceholder.style.backgroundImage = `url('${image}')`;
	}
}

setInterval(getImage, 1000);

eventList = [[topButton, "top"], [downButton, "bottom"], [leftButton, "left"], [rightButton, "right"]];

eventList.forEach((item)=>{
	item[0].addEventListener('pointerdown', ()=>{
		const body = {
			key: item[1],
			pushed: true,
		};
		console.log(body);
		axios.post(backend + "/event", body);
	});
});

eventList.forEach((item)=>{
	item[0].addEventListener('pointerup', ()=>{
		const body = {
			key: item[1],
			pushed: false,
		};
		console.log(body);
		axios.post(backend + "/event", body);
	});
});
