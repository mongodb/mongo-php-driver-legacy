--TEST--
Test for PHP-1231: PHP crashed when using selectCollection inside a Generator
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
<?php if (!version_compare(phpversion(), "5.5", '>=')) echo "skip >= PHP 5.5 needed\n"; ?>
--INI--
mongo.long_as_object=0
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();

function a($host){
    $mongo = new \MongoClient($host);
    $collection = $mongo->selectCollection(dbname(), collname(__FILE__));
    yield 1;
};
 
foreach(a($host) as $file) {
    var_dump($file);
}

echo "ALIVE\n";
?>
--EXPECT--
int(1)
ALIVE
