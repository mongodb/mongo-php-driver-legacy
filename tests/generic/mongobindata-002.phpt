--TEST--
MongoBinData insertion with default type
--DESCRIPTION--
This test expects an E_DEPRECATED notice because the default type will change in
version 2.0 of the extension (see: https://jira.mongodb.org/browse/PHP-407).
--SKIPIF--
<?php require __DIR__ . "/skipif.inc";?>
--FILE--
<?php
error_reporting(E_ALL);

$numNotices = 0;

function handleNotice($errno, $errstr) {
    global $numNotices;
    ++$numNotices;
}

set_error_handler('handleNotice', E_DEPRECATED);

require_once __DIR__ . "/../utils.inc";
$mongo = mongo();
$coll = $mongo->selectCollection('test', 'mongobindata');
$coll->drop();

$coll->insert(array('_id' => 1, 'bin' => new MongoBinData('abcdefg')));

$result = $coll->findOne(array('_id' => 1));

echo get_class($result['bin']) . "\n";
echo $result['bin']->bin . "\n";
echo $result['bin']->type . "\n";
var_dump(1 === $numNotices);
?>
--EXPECT--
MongoBinData
abcdefg
2
bool(true)
