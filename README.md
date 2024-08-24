<p align="center">
    <img style="height: 8em" src="./tools/assets/dinolabsBlue.svg"/>
    <h1 style="margin: 1em">WARR D.I.N.O.labs esp-phenobottle</h1>
</p>

The ESP32 firmware for the WARR spacelabs phenobottle.
This interfaces with the [phenobottle-interface](https://github.com/playduck/phenobottle-interface).

## Build

Building using esp-idf (tested using ESP-IDF v5.4.0):
```bash
idf.py set-target esp32
idf.py menuconfig # set connection parameters
idf.py build flash monitor # -p to specifiy the port
```

### Menuconfig

- Enable PSRAM support (if present on board)
- Set Connection parameters
- Set 4MB flash, 80MHz

### Certificate

generate server site CA cert from a server given at `$URL` by manually extracting it with
```bash
   openssl s_client -showcerts -connect $URL:443 </dev/null
```
and placing the certificate of choice within the chain into `./main/server_root_cert.pem` (including the beginning and ending markers)

## Legal

This project is licensed under the GNU GPLv3.
Find a copy of the license at [COPYING](./COPYING).

---

<p align="center">
    <a href="https://robin-prillwitz.de">Robin Prillwitz MMXXIV</a>
</p>
