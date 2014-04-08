--TEST--
MongoCursor::__construct() error with non-string field names in projection argument
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

/* By coincidence, this legitimate $fields argument is indistinguishable from a
 * numeric array. Legacy behavior requires us to throw an exception, but this
 * test can be changed to no longer expect an exception once we finally remove
 * the legacy behavior.
 *
 * The work-around entails casting $fields to an object, which we do in a
 * related tests for MongoCursor::__construct().
 */
try {
    $document = $collection->findOne(array(), array('0' => 1));
    var_dump($document);
} catch (MongoException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

?>
--EXPECTF--
string(27) "field names must be strings"
int(8)
