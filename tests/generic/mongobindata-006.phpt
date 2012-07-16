--TEST--
MongoBinData insertion with 4MB of binary data
--SKIPIF--
<?php require __DIR__ . "/skipif.inc";?>
--FILE--
<?php
require_once __DIR__ . "/../utils.inc";
$mongo = mongo();
$coll = $mongo->selectCollection(dbname(), 'mongobindata');
$coll->drop();

$data = '';

for ($i = 0; $i < 1024; ++$i) {
    $data .= chr($i % 256);
}

$data = str_repeat($data, 4 * 1024);

$coll->insert(array('_id' => 1, 'bin' => new MongoBinData($data, 0)));

$result = $coll->findOne(array('_id' => 1));

var_dump($data === $result['bin']->bin);
?>
--EXPECTF--
bool(true)
