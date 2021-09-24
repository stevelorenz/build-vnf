# COIN YOLO

# Build and Run

Install ComNetsEmu using script:

```bash
bash ../scripts/install_comnetsemu.sh
```

Run script to fix dependency issue (`eventlet`) for Ryu SDN controller:

```bash
sudo bash ./fix_ryu.sh
```

Build super!!! bloated `coin_yolo` image:
The image size is (unfortunately) larger than **8** GB...
```bash
sudo bash ./build_image.sh
```

Run the topology
