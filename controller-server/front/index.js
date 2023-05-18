const express = require('express');
const cors = require('cors');
const app = express();

app.use(express.static('static'))
app.use(cors());

app.listen(8080, ()=>console.log("Webserver frontend is ready on port 8080"));
