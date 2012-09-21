--TEST--
Manual test for memory leaks and aborted connections.
--SKIPIF--
<?php echo "skip Manual test\n"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../debug.inc";
require_once dirname(__FILE__) . "/../utils.inc";

$m = new Mongo("mongodb://localhost:27018");
$c = $m->selectCollection(dbname(), "test-ping");

$c->drop();
$c->insert( array( 'test' => 'helium' ) );

for ($i = 0; $i < 20; $i++) {
	$c->insert( array( 'test' => "He$i", 'nr' => $i * M_PI ) );
	sleep(1);
	try {
		$r = $c->findOne( array( 'test' => "He$i" ) );
	} catch (Exception $e) {
		exit($e->getMessage());
	}
	echo $r['nr'], "\n";
}

?>
--EXPECTF--
