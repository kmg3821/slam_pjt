const express  = require('express');
const cors     = require('cors');
const fs       = require('fs');
const config   = require('../../config/config.js')
const env      = new config('../../config/config.txt')
const app      = express();

app.use(cors());

app.use(express.static('static'))

app.get('/back', (req, res)=>{
	return res.json({'ip':env.get('BACKEND_IP'), 'port':env.get('BACKEND_PORT')});
})

const PORT = env.get('FRONTEND_PORT')
const HTTPServer = app.listen(PORT, ()=>console.log(`Webserver frontend is ready on port ${PORT}`));
