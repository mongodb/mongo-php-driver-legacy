--TEST--
Test for PHP-684: Add support for socketTimeoutMS
--SKIPIF--
<?php if (version_compare(phpversion(), "5.3.0", "lt")) exit("skip setCallback and closures are 5.3+"); ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$dsn = MongoShellServer::getStandaloneInfo();

printLogs(MongoLog::ALL, MongoLog::ALL, "/timeout/i");

echo "socketTimeoutMS (option)\n";
$m = new MongoClient($dsn, array("socketTimeoutMS" => 1, "connect" => false));

echo "sockettimeoutms lowercased (option)\n";
$m = new MongoClient($dsn, array("socketTimeoutMS" => 2, "connect" => false));

echo "socketTimeoutMS (string)\n";
$m = new MongoClient("localhost/?socketTimeoutMS=42", array("connect" => false));

echo "sockettimeoutms lowercased (string)\n";
$m = new MongoClient("localhost/?sockettimeoutms=52", array("connect" => false));

echo "socketTimeoutMS (string and option)\n";
$m = new MongoClient("localhost/?socketTimeoutMS=62", array("connect" => false, 'socketTimeoutMS' => 72));
?>
--EXPECT--
socketTimeoutMS (option)
- Found option 'socketTimeoutMS': 1
sockettimeoutms lowercased (option)
- Found option 'socketTimeoutMS': 2
socketTimeoutMS (string)
Parsing localhost/?socketTimeoutMS=42
- Found option 'socketTimeoutMS': 42
sockettimeoutms lowercased (string)
Parsing localhost/?sockettimeoutms=52
- Found option 'socketTimeoutMS': 52
socketTimeoutMS (string and option)
Parsing localhost/?socketTimeoutMS=62
- Found option 'socketTimeoutMS': 62
- Found option 'socketTimeoutMS': 72
