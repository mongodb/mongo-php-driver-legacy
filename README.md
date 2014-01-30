[![Build Status](https://travis-ci.org/mongodb/mongo-php-driver.png?branch=master)](https://travis-ci.org/mongodb/mongo-php-driver)

## Installation

To build and install the driver:

    phpize
    ./configure
    make
    sudo make install

Then, add the following to your `php.ini` file:

    extension=mongo.so

### Installing on Windows

Windows builds are available through http://pecl.php.net/package/mongo.

Builds for older driver versions may be found through
https://s3.amazonaws.com/drivers.mongodb.org/php/index.html.

Each driver release includes various builds to support specific versions of PHP
and Windows. Select the correct DLL file for your environment, and add the
following to your `php.ini` file (`VERSION` will vary by environment):

    extension=php_mongo-VERSION.dll

If the DLL is not located within the directory specified by the
[extension_dir](http://www.php.net/manual/en/ini.core.php#ini.extension-dir) INI
setting, you may need to specify its full path.

## Documentation

See [the PHP manual](http://php.net/mongo).


## Support / Feedback

For issues with, questions about, or feedback for the PHP driver, please look
into our [support channels](http://www.mongodb.org/about/support). Please do
not email any of the PHP driver developers directly with issues or
questions—you're more likely to get an answer on the 
[mongodb-user list](http://groups.google.com/group/mongodb-user) on Google
Groups.


## Bugs / Feature Requests

Think you have found a bug? Want to see a new feature in the driver? Please
open a case in our issue management tool, JIRA:

 - Create an account and login (https://jira.mongodb.org).
 - Navigate to [the PHP project](https://jira.mongodb.org/browse/PHP).
 - Click **Create Issue** - Please provide as much information as possible
   about the issue type and how to reproduce it.

Bug reports in JIRA for all driver projects, as well as for the MongoDB server
project, are **public**. Please do not add private information to bug reports.


## Security Vulnerabilities

If you’ve identified a security vulnerability in a driver or any other
MongoDB project, please report it according to the 
[instructions here](http://docs.mongodb.org/manual/tutorial/create-a-vulnerability-report).


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

Taneli Leppä

* Provided a patch for PHP-706 to swap out select() for poll().
