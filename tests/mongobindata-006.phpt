--TEST--
MongoBinData insertion with 4MB of binary data
--FILE--
<?php
$mongo = new Mongo('mongodb://localhost');
$coll = $mongo->selectCollection('test', 'mongobindata');
$coll->drop();

$data = '';

for ($i = 0; $i < 1024; ++$i) {
    $data .= chr($i % 256);
}

$data = str_repeat($data, 4 * 1024);

$coll->insert(array('_id' => 1, 'bin' => new MongoBinData($data)));

$result = $coll->findOne(array('_id' => 1));

var_dump($data === $result['bin']->bin);
?>
--EXPECT--
bool(true)
