--TEST--
MongoClient parses boolean ssl options
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php include 'tests/utils/server.inc'; ?>
<?php

printLogs(MongoLog::PARSE, MongoLog::INFO, "/Found option 'ssl'/");

echo "Testing ssl=true:\n";
new MongoClient('mongodb://localhost:27017/?ssl=true', array('connect' => false));
new MongoClient('mongodb://localhost:27017/?ssl=1', array('connect' => false));
new MongoClient('mongodb://localhost:27017/', array('ssl' => 'true', 'connect' => false));
new MongoClient('mongodb://localhost:27017/', array('ssl' => 1, 'connect' => false));
new MongoClient('mongodb://localhost:27017/', array('ssl' => true, 'connect' => false));

echo "\nTesting ssl=false:\n";
new MongoClient('mongodb://localhost:27017/?ssl=false', array('connect' => false));
new MongoClient('mongodb://localhost:27017/?ssl=0', array('connect' => false));
new MongoClient('mongodb://localhost:27017/?ssl=', array('connect' => false));
new MongoClient('mongodb://localhost:27017/', array('ssl' => 'false', 'connect' => false));
new MongoClient('mongodb://localhost:27017/', array('ssl' => 0, 'connect' => false));
new MongoClient('mongodb://localhost:27017/', array('ssl' => false, 'connect' => false));

?>
===DONE===
--EXPECT--
Testing ssl=true:
- Found option 'ssl': 'true' (parsed as: true)
- Found option 'ssl': '1' (parsed as: true)
- Found option 'ssl': 'true' (parsed as: true)
- Found option 'ssl': '1' (parsed as: true)
- Found option 'ssl': '1' (parsed as: true)

Testing ssl=false:
- Found option 'ssl': 'false' (parsed as: false)
- Found option 'ssl': '0' (parsed as: false)
- Found option 'ssl': '' (parsed as: false)
- Found option 'ssl': 'false' (parsed as: false)
- Found option 'ssl': '0' (parsed as: false)
- Found option 'ssl': '' (parsed as: false)
===DONE===
