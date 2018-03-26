# nlab
nlab is a research tool for teaching and simulating topology and weight evolving artificial neural networks (TWEANN). Written in C++14, licensed under MIT license.

## Install
#### Build
You can find Makefile and VS2017 solution files. C++14 compliant compiler required.
Tested:
* clang 5.0.0
* gcc 7.2.0
* Visual Studio 2017

Works under: linux, cygwin, windows
#### Requirements
* [rapidjson](https://github.com/Tencent/rapidjson)
* [asio](https://github.com/chriskohlhoff/asio) (non-boost version)
* [google benchmark](https://github.com/google/benchmark) (optional)

## Usage
* Launch nlab
* Connect to nlab using [kNUI](https://github.com/Apostol3/knui]) (more info on configuration available in kNUI docs)
* Start training using kNUI
* Launch training environment (e.g. [race_env](https://github.com/Apostol3/race_env))

````
usage: ./nlab [--help] [port (default: 13550)] [connection_uri (default: 'tcp://127.0.0.1:5005')] [net_file]
optional arguments:
  -h, --help            show help message and exit
  port                  port for contolling connection (e.g. from kNUI)
  connection_uri        URI to connect to environment
  net_file              add this network to initial population
````