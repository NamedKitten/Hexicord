Hexicord
=========
*NOTE*: This is a lazy maintenance fork of foxcpp/Hexicord, which seems to be no longer avaliable

[![Build Status](https://travis-ci.org/Chemrat/Hexicord.svg?branch=master)](https://travis-ci.org/Chemrat/Hexicord)
[![License](https://img.shields.io/github/license/Chemrat/Hexicord.svg?style=flat-square)](https://github.com/Chemrat/Hexicord/blob/master/LICENSE)

[Discord API](https://discordapp.com/developers/docs/intro) implementation using C++11 with boost libraries.


### Table of contents 
* [Features](#features)
* [Installation](#installation)
* [Usage](#usage)
* [Documentation](#documentation)
* [Contributing](#contributing)
* [License](#license)


### Features
* Gateway session with auto-reconnection on failure.
* Wrapper that hides weird API details.
* Using HTTP persistent connection to reduce overhead in series of REST requests.
* Minimal runtime dependencies.

### Installation

#### Requirements
* CMake 3.0+
* GCC 4.8.1+ or Clang 3.3+ or other C++11 compatible compiler
* boost.asio 1.56+
* OpenSSL 1.0.1+
* boost.beast (submodule or system)
* _(Optional)_ Doxygen, if you also want to build documentation
* _(Optional)_ Zlib, if you want to use gateway message compression

**Note:** Versions is approximate and can be actually lower or higher, please report any problems with versions in 
specified range.

Following command should be enough for everything on debian-based distros:
```
# apt-get install cmake gcc doxygen libz-dev libboost-dev libboost-system-dev libssl-dev
```

#### Installation
```
$ git clone https://github.com/foxcpp/Hexicord.git  # clone repository
$ cd Hexicord
$ git submodule update --init                       # download boost.beast
$ mkdir build; cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release               # configure build system, Release enabled optimizations.
```
You can also enable `HEXICORD_SHARED` if you need shared library, `HEXICORD_STATIC` is enabled by default.
```
$ make
```

### Usage

Library contain two big classes `Hexicord::GatewayClient` and `Hexicord::Restclient`.
First maintains persistent gateway connection for real-time events, next one provides
interface to REST API methods.

There is simple echo bot:
```cpp
#include <boost/asio/io_service.hpp>
#include <hexicord/gateway_client.hpp>
#include <hexicord/rest_client.hpp>

using namespace Hexicord;

int main() {
    boost::asio::io_service ioService;
    GatewayClient gclient(ioService, "PUT YOUR TOKEN HERE");
    RestClient    rclient(ioService, "PUT YOUR TOKEN HERE");

    gclient.eventsDispatcher.addHandler(Event::MessageCreate, [&client](const nlohmann::json& message) {
        rclient.sendTextMessage(Hexicord::Snowflake(message["channel_id"].get<std::string>()), message["content"]);
    });

    gclient.connect(rclient.getGatewayUrlBot().first); // replace with rclient.getGatewayUrl() if not using bot account.
    ios.run(); 
}
```

### Examples

See [examples/](examples/).

### Documentation

Documentation is generated using [Doxygen](http://www.stack.nl/~dimitri/doxygen/) if `HEXICORD_DOCS` option is enabled 
(default if Doxygen found).

```
$ cmake . -DHEXICORD_DOCS=ON
$ make doc
```
Output will be in `docs/`.

Also documentation for latest release can be browsed online [here](https://foxcpp.github.io/Hexicord).


### Contributing

Here is some of the ways that you can contribute:
* Submit a bug report. We love hearing about broken things, so that we can fix them.
* Provide feedback. Even simple questions about how things work or why they were done a certain way carries value.
* Test code. Checkout `dev` branch, compile it and start trying to break stuff! :-)
* Write code. Check issues marked with 
[**help wanted**](https://github.com/foxcpp/hexicord/issues?q=is%3Aissue+is%3Aopen+label%3A%22help+wanted%22) tag. 

##### Code style
In short: camelCase (snake_case for files), 4 spaces indent, { on same line.
Nobody likes to read long style guides, right? If you unsure about something else - just look at code.

### License 

Distributed under the MIT License.

```
Copyright (c) Maks Mazurov (fox.cpp) 2017 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
