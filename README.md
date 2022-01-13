[![MIT Licensed](https://img.shields.io/github/license/stevelorenz/build-vnf)](https://github.com/stevelorenz/build-vnf/blob/master/LICENSE)

<p align="center">
<img alt="Build-VNF" src="https://github.com/stevelorenz/build-vnf/raw/master/logo/logo_horizontal.png" width="500">
</p>

# Build-VNF

This is just an **experimental**, research and study oriented project to explore new ideas for my PhD thesis (in short, for papers ... 💡) .

## About

**Build-VNF** (Build Virtualized Network Function) is a collection (monorepo 🙈) of sources and tools that I developed and used during my PhD study :wink: (2018-2022) at [ComNets TU Dresden](https://cn.ifn.et.tu-dresden.de/).

When I started my PhD in 2018, SDN/NFV was a totally new topic for me, so this monolithic repository was created to store and manage my prototype/experimental code during my studies.
Since I learned that there are already many popular and mature research and industry projects for NFVI and NFV-MANO components, I decided to investigate the possibility of building fast, energy-efficient and intelligent VNFs for 5G and beyond 😆.

Subdirectories are currently created for sub-projects (ideas and topics) and measurements.
Each subdirectory should contain clear READMEs for their usage.

The structure and code here is completely unstable right now, as I'm not an expert **yet** and am still in the try-error-retry loop 😢.
I plan to create a separate repository after this one has some good and meaningful 🙈 results.
Better, structured documentation will be added later when the project structure is stable.

Thanks to all the open source SDN/NFV projects for teaching me.

## Project Structure

- [FFPP (Fast Forward Packet Processing)](./ffpp/README.md): A fancy 🐼 packet processing engine (library) designed for CNF (Containerized Network Function) and COIN (Computing in the Network) use cases
- COIN_DL: Implementing a COIN-driven distributed deep learning system for in-network video traffic compression
- pktgen: Scripts and tools for software packet generators
- scripts: Utility scripts and helpers

## Citing Our Works

If you like any ideas or tools inside this repository, please consider reading and citing our papers.
These papers have detailed description and rigorous performance evaluation results.

- For [CALVIN (IEEE JSAC)](https://ieeexplore.ieee.org/abstract/document/8672612), X-MAN and FFPP:

```
@inproceedings{xiang2018latency,
  title={Latency measurement of service function chaining on OpenStack platform},
  author={Xiang, Zuo and Gabriel, Frank and Nguyen, Giang T and Fitzek, Frank HP},
  booktitle={2018 IEEE 43rd Conference on Local Computer Networks (LCN)},
  pages={473--476},
  year={2018},
  organization={IEEE}
}

@article{xiang2019reducing,
  title={Reducing latency in virtual machines: Enabling tactile internet for human-machine co-working},
  author={Xiang, Zuo and Gabriel, Frank and Urbano, Elena and Nguyen, Giang T and Reisslein, Martin and Fitzek, Frank HP},
  journal={IEEE Journal on Selected Areas in Communications},
  volume={37},
  number={5},
  pages={1098--1116},
  year={2019},
  publisher={IEEE}
}

@article{xiang2021xman,
  author={Xiang, Zuo and Höweler, Malte and You, Dongho and Reisslein, Martin and Fitzek, Frank H.P.},
  journal={IEEE Transactions on Network and Service Management},
  title={X-MAN: A Non-intrusive Power Manager for Energy-adaptive Cloud-native Network Functions},
  year={2021},
  volume={},
  number={},
  pages={1-1},
  doi={10.1109/TNSM.2021.3126822}
}

@article{Wu2021njica,
  title = {Computing Meets Network: COIN-aware Offloading for Data-intensive Blind Source Separation},
  author = {Huanzhuo {Wu} and Zuo {Xiang} and Giang T. {Nguyen} and Yunbin {Shen} and Frank H. P. {Fitzek}},
  year = {2021},
  date = {2021-09-01},
  journal = {IEEE Network Magazine, Special Issue },
  keywords = {},
  pubstate = {published},
  tppubtype = {article}
}
```

- For COIN_DL and ComNetsEmu:

```
@article{xiang2020comnetsemu,
  title = {An Open Source Testbed for Virtualized Communication Networks},
  author = {Zuo {Xiang} and Sreekrishna {Pandi} and Juan A. {Cabrera Guerrero} and Fabrizio {Granelli} and Patrick {Seeling} and Frank H. P. {Fitzek}},
  year = {2021},
  date = {2021-01-01},
  journal = {IEEE Communications Magazine},
  pages = {1--8},
  note = {(Accepted, 2020)},
  keywords = {},
  pubstate = {published},
  tppubtype = {article}
}

@article{xiang2021you,
  title={You Only Look Once, But Compute Twice: Service Function Chaining for Low-Latency Object Detection in Softwarized Networks},
  author={Xiang, Zuo and Seeling, Patrick and Fitzek, Frank HP},
  journal={Applied Sciences},
  volume={11},
  number={5},
  pages={2177},
  year={2021},
  publisher={Multidisciplinary Digital Publishing Institute}
}
```

## Contributing

This project exists thanks to all people (all my students 😉) who contribute.
The [list](./CONTRIBUTORS.md) of all known contributors.

## Contact

- Zuo Xiang - Maintainer - zuo.xiang@tu-dresden.de or xianglinks@gmail.com (personal)

## License

This project is licensed under the [MIT license](./LICENSE).
