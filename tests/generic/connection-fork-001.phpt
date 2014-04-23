--TEST--
Test for forking and connection management (1)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
<?php if (!function_exists("pcntl_fork")) { exit("skip Requires ext/pcntl"); }?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = mongo_standalone();

$col = $m->selectDb("phpunit")->fork;

$col->drop();
$col->insert(array("parent" => time()), array("w" => 1));

$pid = pcntl_fork();
if ($pid == 0) {
	$col->count();
	exit;
}

$n = 0;
while($n++ < 1000) {
	$col->insert(array("parent" => time()), array("w" => 1));
}

echo $col->count(), "\n";
?>
--EXPECT--
1001
