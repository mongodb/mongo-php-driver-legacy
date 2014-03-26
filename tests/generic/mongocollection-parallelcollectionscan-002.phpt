--TEST--
MongoConnection::parallelCollectionScan() with MultipleIterator
--SKIPIF--
<?php $needs = "2.5.5"; ?>
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$dsn = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($dsn);
$c = $mc->selectCollection(dbname(), collname(__FILE__));
$c->drop();

$notFoundNumbers = array();
foreach (range(1,5000) as $x) {
	$c->insert(array('x' => $x));
	$notFoundNumbers[$x] = true;
}

$document = $c->parallelCollectionScan(8);

$mi = new MultipleIterator(MultipleIterator::MIT_NEED_ANY);

foreach($document as $cursor) {
	$mi->attachIterator( $cursor );
}


foreach($mi as $entries) {
	foreach ($entries as $entry) {
		if ($entry !== NULL) {
			unset( $notFoundNumbers[$entry['x']] );
		}
	}
}
var_dump($notFoundNumbers);
?>
--EXPECTF--
array(0) {
}
