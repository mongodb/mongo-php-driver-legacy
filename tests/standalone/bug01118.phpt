--TEST--
Test for PHP-1118: Aggregate cursor on non-existing collection throws weird error.
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);

$c = $m->demo->foobar;
$c->drop();

$query = array(array('$match' => array( 'foo' => 'telephone' )));

$results = $c->aggregateCursor($query);

foreach ( $results as $r )
{
}
?>
===DONE===
--EXPECT--
===DONE===
