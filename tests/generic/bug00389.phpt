--TEST--
Test for PHP-389: Setting arbitrary flags.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = mongo_standalone();

/* Ensure the collection actually exists. If not, the oplogReplay flag will not
 * cause an "no ts field in query" error on the server.
 */
$m->phpunit->createCollection('bug00389');
$c = $m->phpunit->bug00389;

/* Tailable */
try {
	$cursor = $c->find()->tailable();
	foreach( $cursor as $foo ) { }
} catch ( MongoCursorException $e ) {
	echo $e->getMessage(), "\n";
}

/* Slave okay */
$cursor = $c->find()->slaveOkay();
foreach( $cursor as $foo ) { }

/* Immortal */
$cursor = $c->find()->immortal();
foreach( $cursor as $foo ) { }

/* Await data */
$cursor = $c->find()->awaitData();
foreach( $cursor as $foo ) { }

/* Partial */
$cursor = $c->find()->partial();
foreach( $cursor as $foo ) { }

/* with setFlag() */
for ( $i = 1; $i < 11; $i++ )
{
	echo "Setting flag #", $i, "\n";
	try {
		$cursor = $c->find()->setFlag( $i );
		foreach( $cursor as $foo ) { }
	} catch ( MongoCursorException $e ) {
		echo $e->getMessage(), "\n";
	}
}
?>
--EXPECTF--
%s:%d: tailable cursor requested on non capped collection

%s: Function MongoCursor::slaveOkay() is deprecated in %sbug00389.php on line %d
Setting flag #1
%s:%d: tailable cursor requested on non capped collection
Setting flag #2
Setting flag #3
%s:%d: no ts field in query
Setting flag #4
Setting flag #5
Setting flag #6

Warning: MongoCursor::setFlag(): The CURSOR_FLAG_EXHAUST(6) flag is not supported in %sbug00389.php on line %d

Warning: Invalid argument supplied for foreach() in %sbug00389.php on line %d
Setting flag #7
Setting flag #8
Setting flag #9
Setting flag #10
