--TEST--
Test for PHP-574: Problems with auth-switch and wrong credentials
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$m = mongo();

$db = $m->test2;
$db->authenticate('user2', 'user2' );
$collection = $db->collection;
try
{
	$collection->findOne();
}
catch ( Exception $e )
{
	echo $e->getMessage(), "\n";
}
echo "DONE\n";
?>
--EXPECTF--
Deprecated: Function MongoDB::authenticate() is deprecated in %sbug00574.php on line %d
Failed to connect to: %s:%d: Authentication failed on database 'test2' with username 'user2': auth fails
DONE
