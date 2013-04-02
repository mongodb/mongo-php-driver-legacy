--TEST--
MongoBinData insertion with default type
--DESCRIPTION--
This test expects an E_DEPRECATED notice because the default type will change in
version 2.0 of the extension (see: https://jira.mongodb.org/browse/PHP-407).
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
/* 5.2 */
if (!defined("E_DEPRECATED")) {
    define("E_DEPRECATED", E_STRICT);
}
require_once "tests/utils/server.inc";
error_reporting(-1);

$numNotices = 0;

function handleNotice($errno, $errstr) {
    global $numNotices;
    ++$numNotices;
}

set_error_handler('handleNotice', E_DEPRECATED);

$mongo = mongo_standalone();
$coll = $mongo->selectCollection(dbname(), 'mongobindata');
$coll->drop();

$coll->insert(array('_id' => 1, 'bin' => new MongoBinData('abcdefg')));

$result = $coll->findOne(array('_id' => 1));

echo get_class($result['bin']) . "\n";
echo $result['bin']->bin . "\n";
echo $result['bin']->type . "\n";
var_dump(1 === $numNotices);
?>
--EXPECTF--
MongoBinData
abcdefg
2
bool(true)
