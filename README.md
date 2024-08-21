# esp-phenobottle

## build

building using eps-idf (tested using ESP-IDF v5.2.2).
```bash
idf.py set-target esp32
idf.py menuconfig # set connection parameters
idf.py build flash monitor # -p to specifiy the port
```

## certificate

generate server site CA cert from a server given at `$URL` by manually extracting it with
```bash
   openssl s_client -showcerts -connect $URL:443 </dev/null
```
and placing the certificate of choice within the chain into `./main/server_root_cert.pem` (including the begining and ending markers)
