[![MIT Licensed](https://img.shields.io/github/license/stevelorenz/build-vnf)](https://github.com/stevelorenz/build-vnf/blob/master/LICENSE)

<p align="center">
<img alt="Build-VNF" src="https://github.com/stevelorenz/build-vnf/raw/master/logo/logo_horizontal.png" width="500">
</p>

This is just an **experimental**, research and study oriented project to explore new ideas for my PhD thesis (in short, for papers ... 💡) .

## About

**Build-VNF** (Build Virtualized Network Function) is a collection (monorepo 🙈) of sources and tools that I developed and used during my PhD study :wink: (2018-2022) at [ComNets TU Dresden](https://cn.ifn.et.tu-dresden.de/).

When I started my PhD in 2018, SDN/NFV was a completely new topic for me, so this monolithic repository was created to store and manage my prototype code.

So the structure and code here is totally unstable because I am not expert yet and still in try-error-retry loop 😢.
Also I'm keep exploring new ideas and refine the topic of my PhD thesis, so I just use snippets in this repo to perform
measurements and get fancy plots quickly...
I plan to create separate repositories when there's some stable outcome from this repository.

The project is currently still under **heavy development and revision**.
Better, structured documentation will be added later when the project structure is stable.

## Repo Structure

- FFPP (Fancy Fast Packet Processing): Fancy packet processing engine designed for in-network computing
- COIN_YOLO: DNN based traffic compression
- pktgen: Scripts and tools for software packet generators

## Citing Our Works

If you like any ideas or tools inside this repository, please consider reading and citing our papers.

- For CALVIN and FFPP:

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
```

- For COIN_YOLO:

```
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
The [list](./CONTRIBUTORS.md) of all contributors.

## Contact

- Zuo Xiang - Maintainer - zuo.xiang@tu-dresden.de or xianglinks@gmail.com (personal)

## License

This project is licensed under the [MIT license](./LICENSE).