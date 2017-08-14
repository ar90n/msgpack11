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
| Library | Binary size | time[ms] @ Smallest | time[ms] @ Small | time[ms] @ Medium | time[ms] @ Large | time[ms] @ Largest | Hash |
|----|----|----|----|----|----|----|----|
| msgpack-c-pack(v2.1.4) | 16356 | 2.43 | 10.49 | 107.95 | 1467.11 | 18032.06 | 0f3a2f59 |
| msgpack-c-unpack(v2.1.4) | 79385 | 8.17 | 44.12 | 328.23 | 2644.63 | 28936.23 | f36ed757 |
| msgpack11-pack(v0.0.8) | 667222 | 66.78 | 357.73 | 2926.13 | 27854.59 | 322283.48 | 0f3a2f59 |
| msgpack11-unpack(v0.0.8) | 664792 | 62.21 | 365.62 | 2978.63 | 27399.32 | 313430.80 | f36ed757 |

Git revision : bfbb3d67c4a7f56c28c982f4185d0b54e9326f58

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
