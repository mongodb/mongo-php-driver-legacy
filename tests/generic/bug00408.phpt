--TEST--
Test for PHP-408: MongoBinData custom type is returned as -128.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$coll = $mongo->selectCollection('phpunit', 'mongobindata');
$coll->drop();

$types = array(
    MongoBinData::FUNC,
    MongoBinData::BYTE_ARRAY,
    MongoBinData::UUID,
    MongoBinData::MD5,
    MongoBinData::CUSTOM
);
foreach($types as $type) {
    $doc = array("bin" => new MongoBinData("asdf", $type));
    $coll->insert($doc);
    var_dump($doc["bin"]->type == $type);
}

$cursor = $coll->find();

foreach ($cursor as $result) {
    var_dump($result["bin"]->type);
}
$coll->drop();
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
int(1)
int(2)
int(3)
int(5)
int(128)

