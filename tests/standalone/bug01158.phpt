--TEST--
Test for PHP-1158: Driver emits warning after setting connectTimeoutMS
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

printLogs(MongoLog::PARSE, MongoLog::ALL, "/Found option|Replacing/");

$host = MongoShellServer::getStandaloneInfo();

echo "connectTimeoutMS in query string\n";
new MongoClient(sprintf('mongodb://%s/?connectTimeoutMS=1000', $host));

echo "\nconnectTimeoutMS in options array\n";
new MongoClient($host, array('connectTimeoutMS' => 1000));

echo "\nconnectTimeoutMS in query string and options array\n";
new MongoClient(sprintf('mongodb://%s/?connectTimeoutMS=1000', $host), array('connectTimeoutMS' => 2000));

echo "\ntimeout in query string\n";
new MongoClient(sprintf('mongodb://%s/?timeout=1000', $host));

echo "\ntimeout in options array\n";
new MongoClient($host, array('timeout' => 1000));

echo "\ntimeout in query string and options array\n";
new MongoClient(sprintf('mongodb://%s/?timeout=1000', $host), array('timeout' => 2000));

echo "\nconnectTimeoutMS and timeout in query string and options array\n";
new MongoClient(
    sprintf('mongodb://%s/?connectTimeoutMS=1000&timeout=2000', $host),
    array('connectTimeoutMS' => 3000, 'timeout' => 4000)
);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
connectTimeoutMS in query string
- Found option 'connectTimeoutMS': 1000

connectTimeoutMS in options array
- Found option 'connectTimeoutMS': 1000

connectTimeoutMS in query string and options array
- Found option 'connectTimeoutMS': 1000
- Found option 'connectTimeoutMS': 2000
- Replacing previously set value for 'connectTimeoutMS' (1000)

timeout in query string
- Found option 'timeout' ('connectTimeoutMS'): 1000

timeout in options array
- Found option 'timeout' ('connectTimeoutMS'): 1000

Deprecated: MongoClient::__construct(): The 'timeout' option is deprecated. Please use 'connectTimeoutMS' instead in %s on line %d

timeout in query string and options array
- Found option 'timeout' ('connectTimeoutMS'): 1000
- Found option 'timeout' ('connectTimeoutMS'): 2000
- Replacing previously set value for 'connectTimeoutMS' (1000)

Deprecated: MongoClient::__construct(): The 'timeout' option is deprecated. Please use 'connectTimeoutMS' instead in %s on line %d

connectTimeoutMS and timeout in query string and options array
- Found option 'connectTimeoutMS': 1000
- Found option 'timeout' ('connectTimeoutMS'): 2000
- Replacing previously set value for 'connectTimeoutMS' (1000)
- Found option 'connectTimeoutMS': 3000
- Replacing previously set value for 'connectTimeoutMS' (2000)
- Found option 'timeout' ('connectTimeoutMS'): 4000
- Replacing previously set value for 'connectTimeoutMS' (3000)

Deprecated: MongoClient::__construct(): The 'timeout' option is deprecated. Please use 'connectTimeoutMS' instead in %s on line %d
===DONE===
