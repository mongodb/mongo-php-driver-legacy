[![Build Status](https://travis-ci.org/mongodb/mongo-php-driver.png?branch=master)](https://travis-ci.org/mongodb/mongo-php-driver)

## Installation

To install:

    phpize
    ./configure
    make
    sudo make install

Then add

    extension=mongo.so

to your _php.ini_ file.

## Documentation

See [the PHP manual](http://php.net/mongo).

## Testing

The tests are not available as part of the PECL package, but they are available 
on [Github](http://www.github.com/mongodb/mongo-php-driver/tree/master/tests).  

See [CONTRIBUTING.md](CONTRIBUTING.md) for how to run and create new tests.

## Credits

Jon Moss

* Came up with the idea and implemented `MongoCursor` implementing `Iterator`

Pierre-Alain Joye

* Helped build the Windows extension and has provided the VC6 builds

Cesar Rodas

* Created the `MongoCursor::info` method
* Implemented GridFS read streaming

William Volkman

* Made connection code check & handle error status

Derick Rethans

* Implemented `MongoInt32`, `MongoInt64` and related _php.ini_ options.
