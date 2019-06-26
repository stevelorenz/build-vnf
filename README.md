# Build VNF #

This repository is under heavy development. The directories are not cleanly and clearly organized. Redundant directories
will be removed before the beta 1.0 version. The README will also be updated.

This project is hosted on [GitHub](https://github.com/stevelorenz/build-vnf) and
[BitBucket](https://bitbucket.org/comnets/build-vnf/src/master/), please create an issue or a pull request if you have
any questions or code contribution.

## Introduction ##

This repository is a collection of programs/tools used for my PhD topic: Ultra Low Latency Network Softwarization.

These programs are used to build, test and evaluate different packet IO and processing tools for implementing
Virtualized Network Function (VNFs) on virtual machines, Unikernels or containers. The main focus of performance here is
the **low latency** (End-to-End service delay). Pros and cons of each tool should be studied and latter utilized for
building VNF with different requirements.  For example, firewalls or load balancers can be built using tools that
provide high IO speeds but limited computing power.  For compute-intensive network functions, such as data encryption,
network coding or compressed sensing. The processing ability and delay performance of the tool should also be
considered.

## Catalog ##

>  TODO:  <25-06-19, zuo> >  Add detailed Catalog

1. [CALVIN](./CALVIN/): Programs and scripts used in [JSAC CALVIN paper](https://ieeexplore.ieee.org/abstract/document/8672612).
2. [fastio_user](./fastio_user/): A C library for packet IO and processing in user space.
3. ...



## Documentation ##

This repository contains a tutorial/Wiki like documentation whose source files (use reStructuredText format) are located
in [./doc/source/](./doc/source/). [Sphinx](http://www.sphinx-doc.org/en/master/) is used to build the documentation.
Install Sphinx before building the documentation source files. On a Unix-like system, the documentation (HTML format)
can be built and opened with:

```bash
$ make doc-html
$ xdg-open ./doc/build/html/index.html
```

## Contributors ##

1. Zuo Xiang: Maintainer
2. Funing Xia: Add Lua scripts for latency measurement with MoonGen.
3. Renbing Zhang: Add YOLO-based object detection pre-processing APP
