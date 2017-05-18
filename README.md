[![Build Status](https://travis-ci.org/ar90n/msgpack11.svg?branch=master)](https://travis-ci.org/ar90n/msgpack11)

What is msgpack11 ?
===============

msgpack11 is a tiny MsgPack library for C++11, providing MsgPack parsing and serialization.  
This library is inspired by [json11](https://github.com/dropbox/json11).  
The API of msgpack11 is designed to be similar with json11.

Installation
===============
* Using CMake

        git clone git@github.com:ar90n/msgpack11.git
        mkdir build
        cd build
        cmake ../msgpack11
        make && make install

* Using Buck

        git clone git@github.com:ar90n/msgpack11.git
        cd msgpack11
        buck build :msgpack11

Example
===============

    MsgPack my_msgpack = MsgPack::object {
        { "key1", "value1" },
        { "key2", false },
        { "key3", MsgPack::array { 1, 2, 3 } },
    };
    std::string msgpack_bytes = my_msgpack.dump();

Feature
===============
* Support serialization and deserialization.

Acknowledgement
===============
* [json11](https://github.com/dropbox/json11)
* [msgpack-c](https://github.com/msgpack/msgpack-c)

License
===============
This software is released under the MIT License, see LICENSE.txt.
