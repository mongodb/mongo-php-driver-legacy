--TEST--
MongoLog generates E_NOTICE messages
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc";?>
--INI--
display_errors=1
--FILE--
<?php
error_reporting(E_ALL);

$numNotices = 0;

function handleNotice($errno, $errstr) {
    global $numNotices;
    ++$numNotices;
}

set_error_handler('handleNotice', E_NOTICE);

MongoLog::setLevel(MongoLog::IO);
MongoLog::setModule(MongoLog::FINE);

require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
$coll = $mongo->selectCollection(dbname(), 'mongolog');
$coll->drop();

$coll->insert(array('x' => 1));

var_dump(0 < $numNotices);
--EXPECT--
bool(true)
