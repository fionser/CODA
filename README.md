CODA
====
---

## Overview
Cryptographically secure Of statistical Data Analaysis.

## Description
* core - core is an engine to encrypt decrypt eval, written in c++.
* ui/client - client is an manager to comunicate, resources like filesystem on the client computer.  written in c++.
* ui/server - server is an manager to comunicate, resources like filesystem, on the server. written in c++.

## Requirement
* Linux (g++ 4.9+, cmake 3.2+, gnu make 3.8+, perl v5.18.2+, m4 v1.4.6+)
* Mac OSX (llvm 6.1+, cmake 3.2+, gnu make 3.8+, perl v5.18.2+, m4 v1.4.6+)

## Install
	bash build.sh
	cd build/bin

For Linux platforms, we can use multi-threads programming to speed up the evaluation.
`bash build.sh` will ask you to determine the number of threads to use. It writes as
`This platform can use multi-threads. Please set the number of threads:`
and then type into a positive number.

## Licence
CODA is distributed under the terms of the [GNU General Public License ](https://www.gnu.org/licenses/gpl.html) (GPL).


## Usage
See this [pages](https://fionser.github.io/CODA).
