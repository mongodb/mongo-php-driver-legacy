--TEST--
Test for PHP-683: Add support for the connectTimeoutMS connection parameter.
--SKIPIF--
<?php if (version_compare(phpversion(), "5.3.0", "lt")) exit("skip setCallback and closures are 5.3+"); ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$dsn = MongoShellServer::getStandaloneInfo();

printLogs(MongoLog::ALL, MongoLog::ALL, "/(Found option.*imeout*)|Replacing/");

echo "timeout only\n";
$m = new MongoClient($dsn, array("connect" => false, "timeout" => 1));

echo "timeout and connectTimeoutMS\n";
$m = new MongoClient($dsn, array("connect" => false, "timeout" => 2, "connectTimeoutMS" => 3));

echo "connectTimeoutMS only\n";
$m = new MongoClient($dsn, array("connect" => false, "connectTimeoutMS" => 4));

echo "connectTimeoutMS and timeout\n";
$m = new MongoClient($dsn, array("connect" => false, "connectTimeoutMS" => 5, "timeout" => 6));

echo "connecttimeoutms lowercased\n";
$m = new MongoClient($dsn, array("connect" => false, "connecttimeoutms" => 7));

?>
--EXPECTF--
timeout only
- Found option 'timeout' ('connectTimeoutMS'): 1

%s: MongoClient::__construct(): The 'timeout' option is deprecated. Please use 'connectTimeoutMS' instead in %s on line %d
timeout and connectTimeoutMS
- Found option 'timeout' ('connectTimeoutMS'): 2

%s: MongoClient::__construct(): The 'timeout' option is deprecated. Please use 'connectTimeoutMS' instead in %s on line %d
- Replacing previously set value for 'connectTimeoutMS' (2)
- Found option 'connectTimeoutMS': 3
connectTimeoutMS only
- Found option 'connectTimeoutMS': 4
connectTimeoutMS and timeout
- Found option 'connectTimeoutMS': 5
- Replacing previously set value for 'connectTimeoutMS' (5)
- Found option 'timeout' ('connectTimeoutMS'): 6

%s: MongoClient::__construct(): The 'timeout' option is deprecated. Please use 'connectTimeoutMS' instead in %s on line %d
connecttimeoutms lowercased
- Found option 'connectTimeoutMS': 7
