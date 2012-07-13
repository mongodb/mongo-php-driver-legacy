--TEST--
MongoLog generates E_NOTICE messages
--FILE--
<?php
error_reporting(E_ALL);
ini_set('display_errors', 1);

$numNotices = 0;

function handleNotice($errno, $errstr) {
    global $numNotices;
    ++$numNotices;
}

set_error_handler('handleNotice', E_NOTICE);

MongoLog::setLevel(MongoLog::IO);
MongoLog::setModule(MongoLog::FINE);

$mongo = new Mongo('mongodb://localhost');
$coll = $mongo->selectCollection('test', 'mongolog');
$coll->drop();

$coll->insert(array('x' => 1));

var_dump($numNotices > 0);
--EXPECT--
bool(true)
