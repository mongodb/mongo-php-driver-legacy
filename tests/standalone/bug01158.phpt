--TEST--
Test for PHP-1158: Driver emits warning after setting connectTimeoutMS
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
<?php if (!version_compare(phpversion(), "5.3", '>=')) echo "skip >= PHP 5.3 needed\n"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

printLogs(MongoLog::PARSE, MongoLog::ALL, "/Found option|Replacing/");

$host = MongoShellServer::getStandaloneInfo();

echo "connectTimeoutMS in query string\n";
new MongoClient(sprintf('mongodb://%s/?connectTimeoutMS=1', $host));

echo "\nconnectTimeoutMS in options array\n";
new MongoClient($host, array('connectTimeoutMS' => 1));

echo "\nconnectTimeoutMS in query string and options array\n";
new MongoClient(sprintf('mongodb://%s/?connectTimeoutMS=1', $host), array('connectTimeoutMS' => 2));

echo "\ntimeout in query string\n";
new MongoClient(sprintf('mongodb://%s/?timeout=1', $host));

echo "\ntimeout in options array\n";
new MongoClient($host, array('timeout' => 1));

echo "\ntimeout in query string and options array\n";
new MongoClient(sprintf('mongodb://%s/?timeout=1', $host), array('timeout' => 2));

echo "\nconnectTimeoutMS and timeout in query string and options array\n";
new MongoClient(
    sprintf('mongodb://%s/?connectTimeoutMS=1&timeout=2', $host),
    array('connectTimeoutMS' => 3, 'timeout' => 4)
);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
connectTimeoutMS in query string
- Found option 'connectTimeoutMS': 1

connectTimeoutMS in options array
- Found option 'connectTimeoutMS': 1

connectTimeoutMS in query string and options array
- Found option 'connectTimeoutMS': 1
- Found option 'connectTimeoutMS': 2
- Replacing previously set value for 'connectTimeoutMS' (1)

timeout in query string
- Found option 'timeout' ('connectTimeoutMS'): 1

timeout in options array
- Found option 'timeout' ('connectTimeoutMS'): 1

Deprecated: MongoClient::__construct(): The 'timeout' option is deprecated. Please use 'connectTimeoutMS' instead in %s on line %d

timeout in query string and options array
- Found option 'timeout' ('connectTimeoutMS'): 1
- Found option 'timeout' ('connectTimeoutMS'): 2
- Replacing previously set value for 'connectTimeoutMS' (1)

Deprecated: MongoClient::__construct(): The 'timeout' option is deprecated. Please use 'connectTimeoutMS' instead in %s on line %d

connectTimeoutMS and timeout in query string and options array
- Found option 'connectTimeoutMS': 1
- Found option 'timeout' ('connectTimeoutMS'): 2
- Replacing previously set value for 'connectTimeoutMS' (1)
- Found option 'connectTimeoutMS': 3
- Replacing previously set value for 'connectTimeoutMS' (2)
- Found option 'timeout' ('connectTimeoutMS'): 4
- Replacing previously set value for 'connectTimeoutMS' (3)

Deprecated: MongoClient::__construct(): The 'timeout' option is deprecated. Please use 'connectTimeoutMS' instead in %s on line %d
===DONE===
