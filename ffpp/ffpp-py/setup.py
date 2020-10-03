#!/usr/bin/env python3

"""Setuptools Configuration"""

from os.path import join
from setuptools import setup, find_packages

modname = distname = "ffpp"

setup(
    name=distname,
    version="0.1.0",
    description="Fast Functional Packet Processing.",
    author="Zuo Xiang",
    author_email="zuo.xiang@tu-dresden.de",
    packages=find_packages(exclude=[]),
    long_description="""
        Fast Functional Packet Processing.
        """,
    classifiers=[
        #   3 - Alpha
        #   4 - Beta
        #   5 - Production/Stable
        "Programming Language :: Python:: 3.6",
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "Topic :: System :: DPDK",
        "Natural Language :: English",
        "License :: MIT License",
    ],
    keywords="FFPP DPDK",
    setup_requires=["cffi>=1.14,<2.0"],
    cffi_modules=["ffpp_builder.py:ffibuilder"],
    install_requires=["setuptools>=39.0.1,<40.0.0", "cffi>=1.14,<2.0"],
)
