--TEST--
Test for PHP-537: 
--FILE--
<?php
$m = new MongoClient;
$c = $m->phpunit->test;

echo "This document should be just too large: ";
$d = array();
$d['content'] = str_repeat('x', 16 * 1024 * 1024);
try {
	$c->insert($d);
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}

echo "This document should just fit: ";
$d = array();
$d['content'] = str_repeat('x', 16 * 1024 * 1024 - 36);
try {
	$c->insert($d);
	echo "it fit!\n";
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}

echo "Batch insert with 4 documents: ";

try {
	$c->batchInsert(array($d, $d, $d, $d));
	echo "it fit!\n";
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}
?>
--EXPECTF--
This document should be just too large: document fragment is too large: 16777252, max: 16777216
This document should just fit: it fit!
Batch insert with 4 documents: current batch size is too large: 50331681, max: 48000000
