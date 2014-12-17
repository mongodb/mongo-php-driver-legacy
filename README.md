[![Build Status](https://travis-ci.org/mongodb/mongo-php-driver.png?branch=master)](https://travis-ci.org/mongodb/mongo-php-driver)

## Installation

To build and install the driver:

    phpize
    ./configure
    make
    sudo make install

Then, add the following to your `php.ini` file:

    extension=mongo.so

### Enabling enterprise features

To connect to MongoDB Enterprise using SASL (GSSAPI) or LDAP (PLAIN) you need
to build the driver against cyrus-sasl2 (external library).
This is done by passing --with-mongo-sasl to ./configure, optionally passing
in the directory to where cyrus-sasl2 was installed:

    phpize
    ./configure --with-mongo-sasl=/usr/local
    make
    sudo make install

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


## How To Ask For Help

When asking for support, or while providing feedback in the form of bugs or
feature requestes, please include the following relevant information:

 - Detailed steps on how to reproduce the problem, including a script that
   reproduces your problem, if possible. 
 - The exact PHP version used. You can find this by running `php -v` on the
   command line, or by checking the output of `phpinfo();` in a script
   requested through a web server.
 - The exact version of the MongoDB driver for PHP. You can find this by
   running `php --ri mongo | grep Version` on the command line, or by running
   a script containing `<?php echo phpversion("mongo"); ?>`.
 - The operating system and version (e.g. Windows 7, OSX 10.8, ...).
 - How you installed the driver, either via your distribution's package
   management, with "brew", with an \*AMP installation package, or through a
   manual source compile and install.
 - With connection and replica set selection issues, please also provide a
   full debug log. See further down on how to make one.



### Support / Feedback

For issues with, questions about, or feedback for the PHP driver, please look
into our [support channels](http://www.mongodb.org/about/support). Please do
not email any of the PHP driver developers directly with issues or
questions—you're more likely to get an answer on the 
[mongodb-user list](http://groups.google.com/group/mongodb-user) on Google
Groups.


### Bugs / Feature Requests

Think you have found a bug? Want to see a new feature in the driver? Please
open a case in our issue management tool, JIRA:

 - Create an account and login (https://jira.mongodb.org).
 - [Create a new issue](https://jira.mongodb.org/secure/CreateIssue!default.jspa?pid=10007).

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


## Full debug log

You can make a full debug log by using the following code:


```php
<?php
/*
 * This script will catch all internal driver logging for MongoDB and log them
 * to a file /tmp/MONGO-PHP-LOG.<unix-timestamp>. This log will give the
 * driver developers more or less all the information they'll need to debug
 * this issue
 */
 
function module2string($module)
{
    switch ($module) {
        case MongoLog::RS: return "REPLSET";
        case MongoLog::CON: return "CON";
        case MongoLog::IO: return "IO";
        case MongoLog::SERVER: return "SERVER";
        case MongoLog::PARSE: return "PARSE";
        default: return $module;
    }
}
 
function level2string($level)
{
    switch ($level) {
        case MongoLog::WARNING: return "WARN";
        case MongoLog::INFO: return "INFO";
        case MongoLog::FINE: return "FINE";
        default: return $level;
    }
}
 
function logMongo($module, $level, $message, $save = false) {
    static $log = "";
    $log .= sprintf("%s | %s (%s): %s\n", date('Y-m-d H:i:s',time()), module2string($module), level2string($level), $message);
 
    if ($save) {
        file_put_contents("/tmp/MONGO-PHP-LOG." . time(), $log);
        $log = "";
    }
}
 
function saveMongoLogException($ex) {
    if ($ex instanceof MongoException) {
        $msg = sprintf("Uncaught exception: %s, %s\n",  get_class($ex), $ex->getMessage());
        logMongo(get_class($ex), "ERROR", $ex->getMessage(), true);
    }
    throw $ex;
}

function saveMongoLog() {
    logMongo("EXIT", "EXIT", "EXIT", true);
}
 
MongoLog::setLevel(MongoLog::ALL);
MongoLog::setModule(MongoLog::ALL);
MongoLog::setCallback("logMongo");

register_shutdown_function("saveMongoLog");
 
/* If an global exception handler is used, or a default exception handler is
 * already registered, please comment out this line and put into your catch
 * block: saveMongoLogException($exception); */
set_exception_handler("saveMongoLogException");
?>
```


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
