--TEST--
MongoBinData insertion with default type
--FILE--
<?php
$mongo = new Mongo('mongodb://localhost');
$coll = $mongo->selectCollection('test', 'mongobindata');
$coll->drop();

$coll->insert(array('_id' => 1, 'bin' => new MongoBinData('abcdefg')));

$result = $coll->findOne(array('_id' => 1));

echo get_class($result['bin']) . "\n";
echo $result['bin']->bin . "\n";
echo $result['bin']->type . "\n";
?>
--EXPECT--
MongoBinData
abcdefg
2
