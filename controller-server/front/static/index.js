const modelServer = "127.0.0.1"
const modelPort   = 8081

const imagePlaceholder = document.querySelector(".camera");

const getImage = async function(){
	let ok = false;
	let imageUrl = null;
	await axios.get("http://" + modelServer + ":" + modelPort + "/image/recent")
	.then((res)=>{
		console.log(res);
		if(res.status == 200){
			this.ok = true;
		}
	})
	.catch((error)=>{
		this.ok = false;
	});
	if(ok){
		//const image    = axios.get(modelServer + ":" + modelPort + imageUrl);
		//imagePlaceholder.style.backgroundImage = `url('${image}')`;
	}
}

setInterval(getImage, 1000);
