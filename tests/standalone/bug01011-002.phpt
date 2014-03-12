--TEST--
Test for PHP-1011: MongoClient memory leak parsing multiple write concern string mode options
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

MongoLog::setModule(MongoLog::PARSE);
MongoLog::setLevel(MongoLog::INFO);

$mc = new MongoClient(
    sprintf('mongodb://%s/?w=foo', MongoShellServer::getStandaloneInfo()),
    array('w' => 'bar')
);

$db = $mc->selectDB(dbname());

dump_these_keys($db->getWriteConcern(), array('w'));

?>
===DONE===
--EXPECTF--
Notice: PARSE   INFO: Parsing mongodb://%s/?w=foo in %s on line %d

Notice: PARSE   INFO: - Found node: %s:%d in %s on line %d

Notice: PARSE   INFO: - Connection type: STANDALONE in %s on line %d

Notice: PARSE   INFO: - Found option 'w': 'foo' in %s on line %d

Notice: PARSE   INFO: - Found option 'w': 'bar' in %s on line %d
array(1) {
  ["w"]=>
  string(3) "bar"
}
===DONE===
