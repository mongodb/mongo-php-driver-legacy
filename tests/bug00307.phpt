--TEST--
Test for bug PHP-307: getHosts() turns wrong results.
--SKIP--
<?php echo "skip: no idea how to test against replicasets yet"; ?>
--FILE--
<?php
$m = new Mongo('mongodb://127.0.0.1', array('replicaSet' => 'a'));
$d = $m->phpunit;
var_dump($c->findOne());
var_dump($m->getHosts());
?>
--EXPECT--
