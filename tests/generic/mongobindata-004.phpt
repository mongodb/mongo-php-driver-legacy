--TEST--
MongoBinData insertion with various types
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$coll = $mongo->selectCollection(dbname(), 'mongobindata');
$coll->drop();

$coll->insert(array('bin' => new MongoBinData('pqr', MongoBinData::GENERIC)));
$coll->insert(array('bin' => new MongoBinData('abc', MongoBinData::FUNC)));
$coll->insert(array('bin' => new MongoBinData('def', MongoBinData::BYTE_ARRAY)));
$coll->insert(array('bin' => new MongoBinData('ghi', MongoBinData::UUID)));
$coll->insert(array('bin' => new MongoBinData('stu', MongoBinData::UUID_RFC4122)));
$coll->insert(array('bin' => new MongoBinData('jkl', MongoBinData::MD5)));
$coll->insert(array('bin' => new MongoBinData('mno', MongoBinData::CUSTOM)));


$cursor = $coll->find();

foreach ($cursor as $result) {
    printf("Type %d with data \"%s\"\n", $result['bin']->type, $result['bin']->bin);
}
?>
--EXPECT--
Type 0 with data "pqr"
Type 1 with data "abc"
Type 2 with data "def"
Type 3 with data "ghi"
Type 4 with data "stu"
Type 5 with data "jkl"
Type 128 with data "mno"
