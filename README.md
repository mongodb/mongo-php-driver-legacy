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
To run the test, you'll have to configure the tests/mongo-test-cfg.inc file
(copy the tests/mongo-test-cfg.inc.template to tests/mongo-test-cfg.inc and edit it).
By default, the tests will be executed in several different environments;
(currently:)
* Replicaset (without authentication)
* Standalone

If you are not running any of these environments, you can disable these runs by
adjusting the configuration file.


Once you have filled out the configuration template, run the following command to
launch the test suite.

    $ make test


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
