#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Handle feature maps
"""

import sklearn

pca = sklearn.decomposition.PCA()
pca.fit(X)
X_pca = pca.transform(X)
X_reconstructed = pca.reverse_transform(X_pca)
