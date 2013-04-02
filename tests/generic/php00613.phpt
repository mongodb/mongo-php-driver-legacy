--TEST--
Test for PHP-613: Add support for wTimeoutMS in the connection string as per the spec 
--SKIPIF--
<?php if (version_compare(phpversion(), "5.3.0", "lt")) exit("skip setCallback and closures are 5.3+"); ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$dsn = MongoShellServer::getStandaloneInfo();
printLogs(MongoLog::ALL, MongoLog::ALL, "/(Found option.*imeout*)|Replacing/");

echo "wTimeout only\n";
$m = new MongoClient($dsn, array("connect" => false, "wTimeout" => 1));

echo "wTimeout and wTimeoutMS\n";
$m = new MongoClient($dsn, array("connect" => false, "wTimeout" => 2, "wTimeoutMS" => 3));

echo "wTimeoutMS only\n";
$m = new MongoClient($dsn, array("connect" => false, "wTimeoutMS" => 4));

echo "wTimeoutMS and wTimeout\n";
$m = new MongoClient($dsn, array("connect" => false, "wTimeoutMS" => 5, "wTimeout" => 6));

echo "wtimeoutms lowercased\n";
$m = new MongoClient($dsn, array("connect" => false, "wtimeoutms" => 7));

?>
--EXPECT--
wTimeout only
- Found option 'wTimeout' ('wTimeoutMS'): 1
wTimeout and wTimeoutMS
- Found option 'wTimeout' ('wTimeoutMS'): 2
- Replacing previously set value for 'wTimeoutMS' (2)
- Found option 'wTimeoutMS': 3
wTimeoutMS only
- Found option 'wTimeoutMS': 4
wTimeoutMS and wTimeout
- Found option 'wTimeoutMS': 5
- Replacing previously set value for 'wTimeoutMS' (5)
- Found option 'wTimeout' ('wTimeoutMS'): 6
wtimeoutms lowercased
- Found option 'wTimeoutMS': 7
