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

    //access to elements
    std::cout << my_msgpack["key1"].string_value();

    //serialize
    std::string msgpack_bytes = my_msgpack.dump();

    //deserialize
    std::string err;
    MsgPack des_msgpack = MsgPack::parse(msgpack_bytes, err);

There are more specific examples in example.cpp.
Please see it.

Benchmark
===============
Derived from [schemaless-benchmarks](https://github.com/ludocode/schemaless-benchmarks)

| Library | Binary size | time[ms] @ Smallest | time[ms] @ Small | time[ms] @ Medium | time[ms] @ Large | time[ms] @ Largest |
|----|----|----|----|----|----|----|
| msgpack-c-pack(v2.1.4) | 6649 | 0.55 | 2.38 | 43.22 | 711.75 | 8748.20 |
| msgpack-c-unpack(v2.1.4) | 21804 | 1.34 | 6.00 | 83.09 | 714.64 | 11192.32 |
| msgpack11-pack(v0.0.9) | 99844 | 20.80 | 130.04 | 1063.24 | 10466.65 | 136640.99 |
| msgpack11-unpack(v0.0.9) | 99460 | 13.31 | 92.54 | 786.73 | 7345.43 | 99119.56 |

CPU : 2.6 GHz Intel Core i7  
Memory : 16 GB 2133 MHz LPDDR3  
Git revision : 6f6b4302b68b3c88312eb24367418b7fce81298c

Feature
===============
* Support serialization and deserialization.

Acknowledgement
===============
* [json11](https://github.com/dropbox/json11)
* [msgpack-c](https://github.com/msgpack/msgpack-c)
* [schemaless-benchmarks](https://github.com/ludocode/schemaless-benchmarks)

License
===============
This software is released under the MIT License, see LICENSE.txt.
