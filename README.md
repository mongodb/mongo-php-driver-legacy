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

See [the PHP manual](http://us.php.net/manual/en/book.mongo.php).

## Testing

The tests are not available as part of the PECL package, but they are available 
on [Github](http://www.github.com/mongodb/mongo-php-driver/tree/master/tests).  
They use [PHPUnit](from http://www.phpunit.de).  To run the tests:

    $ phpunit tests/MongoSuite.php

There must be a mongod instance running on port 27017 in order to run the tests.
They also assume that php.ini sets error_reporting to `E_STRICT | E_ALL`.

The tests will spit out a bunch of warnings if you do not have 
_mongo-php-driver/php_ on your include path, but it will just skip the tests that 
require that.

The tests will also attempt to create an admin user using the shell.  If 
_mongo_ is not on your path, you will get some output complaining about it but
those tests will just be skipped.

## Credits

Jon Moss

* Came up with the idea and implemented `MongoCursor` implementing `Iterator`

Pierre-Alain Joye

* Helped build the Windows extension and has provided the VC6 builds

Cesar Rodas

* Created the `MongoCursor::info` method

William Volkman

* Made connection code check & handle error status

Derick Rethans

* Implemented `MongoInt32`, `MongoInt64` and related _php.ini_ options.
