--TEST--
MongoCursor::__construct() error with non-string field names in legacy projection argument
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$collection = $mc->selectCollection(dbname(), collname(__FILE__));

$collection->drop();
$collection->insert(array('0' => 'zero', '1' => 'one'));

/* Legacy style is an array of field names to include. An exception will be
 * thrown if the field names are not strings. Support for this argument style
 * should be considered deprecated and subject to removal in the future.
 */
try {
    $document = $collection->findOne(array(), array(0));
    var_dump($document);
} catch (MongoException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

?>
--EXPECTF--
string(27) "field names must be strings"
int(8)
