--TEST--
MongoClient does not support ssl "prefer" option
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php include 'tests/utils/server.inc'; ?>
<?php

printLogs(MongoLog::PARSE, MongoLog::INFO, "/Found option 'ssl'/");

echo "Testing ssl=prefer:\n";

try {
    new MongoClient('mongodb://localhost:27017/?ssl=prefer', array('connect' => false));
} catch (MongoConnectionException $e) {
    printf("Exception: %s\n", $e->getMessage());
}

try {
    new MongoClient('mongodb://localhost:27017/?ssl=2', array('connect' => false));
} catch (MongoConnectionException $e) {
    printf("Exception: %s\n", $e->getMessage());
}

try {
    new MongoClient('mongodb://localhost:27017/', array('ssl' => 'prefer', 'connect' => false));
} catch (MongoConnectionException $e) {
    printf("Exception: %s\n", $e->getMessage());
}

try {
    new MongoClient('mongodb://localhost:27017/', array('ssl' => 2, 'connect' => false));
} catch (MongoConnectionException $e) {
    printf("Exception: %s\n", $e->getMessage());
}

?>
===DONE===
--EXPECT--
Testing ssl=prefer:
- Found option 'ssl': 'prefer'
Exception: SSL=prefer is currently not supported by mongod
- Found option 'ssl': '2'
Exception: SSL=prefer is currently not supported by mongod
- Found option 'ssl': 'prefer'
Exception: SSL=prefer is currently not supported by mongod
- Found option 'ssl': '2'
Exception: SSL=prefer is currently not supported by mongod
===DONE===
