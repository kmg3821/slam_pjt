const fs = require('fs')
class env{
	constructor(fileSrc){
		this.environments={};
		const data = fs.readFileSync(fileSrc, 'utf-8');
		data.split('\n').forEach((environment)=>{
			if(environment!==''){
				const key = environment.split('=')[0];
				const val = environment.split('=')[1];
				this.environments[key] = val;
			}
		});
	}
	get(env_name){
		return this.environments[env_name];
	}
}

module.exports = env;
