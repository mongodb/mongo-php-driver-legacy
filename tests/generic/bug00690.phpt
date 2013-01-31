--TEST--
Test for PHP-690: Percentage symbol is not escaped in error messages
--SKIPIF--
<?php require_once dirname(__FILE__) . '/skipif.inc'; ?>
--FILE--
<?php require_once dirname(__FILE__) . '/../utils.inc'; ?>
<?php
$m = new_mongo();
$c = $m->selectCollection(dbname(), 'bug690');
$c->drop();

$c->insert(array('_id'=>'hello%20London', 'added'=>time()));
try
{
	$c->insert(array('_id'=>'hello%20London', 'added'=>time()));
}
catch ( Exception $e )
{
	echo $e->getMessage(), "\n";
}

?>
--EXPECTF--
%s:%d: E11000 duplicate key error index: %s dup key: { : "hello%20London" }
