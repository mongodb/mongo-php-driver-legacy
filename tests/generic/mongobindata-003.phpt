--TEST--
MongoBinData insertion with null bytes, control characters and symbols
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
$coll = $mongo->selectCollection(dbname(), 'mongobindata');
$coll->drop();

$coll->insert(array('bin' => new MongoBinData(str_repeat(chr(0), 3), 0)));
$coll->insert(array('bin' => new MongoBinData(chr(1) . chr(2) . chr(3) . chr(4), 0)));
$coll->insert(array('bin' => new MongoBinData(chr(255) . chr(7) . chr(199), 0)));

$cursor = $coll->find();

foreach ($cursor as $result) {
    $numBytes = strlen($result['bin']->bin);
    $bytes = array();

    for ($i = 0; $i < $numBytes; ++$i) {
        $bytes[] = ord($result['bin']->bin[$i]);
    }

    printf("%d bytes: %s\n", $numBytes, implode(',', $bytes));
}
?>
--EXPECT--
3 bytes: 0,0,0
4 bytes: 1,2,3,4
3 bytes: 255,7,199
