const path = require('path');
const express = require('express');
const multer = require('multer');
const app = express();

const storage = multer.diskStorage({
    destination: (req, file, cb) => {
        cb(null, 'uploads/');
    },
    filename: (req, file, cb) => {
        cb(null, Date.now() + '-' + file.originalname);
    },
});

const upload = multer({ storage: storage });

app.post(
    '/api/v1/image', upload.single('image'), async (req, res) => {
      console.log(req)
      const device_id = req.header('Device-Id');
      const timestamp = req.header('Timestamp');
      const image_mime = req.header('Form-Mime');

      if (!req.file) {
        return res.status(400).send('No files were uploaded.');
      }
      if (!device_id || !timestamp || !image_mime) {
        return res.status(400).send('Invalid request');
      }

      console.log(device_id);
      console.log(timestamp);
      console.log(image_mime);

      res.send({state: "success"});
});

const port = 8080;
app.listen(port, () => {
    console.log("listening on", port);
});
