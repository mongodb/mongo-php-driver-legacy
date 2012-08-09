--TEST--
Test for forking and connection management
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
<?php if (!function_exists("pcntl_fork")) { exit("skip Requires ext/pcntl"); }?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$m = mongo();

$col = $m->selectDb("phpunit")->fork;

$col->drop();
$col->insert(array("parent" => time()), array("safe" => 1));

$pid = pcntl_fork();
if ($pid == 0) {
	$col->count();
	exit;
}

$n = 0;
while($n++ < 1000) {
	$col->insert(array("parent" => time()), array("safe" => 1));
}

echo $col->count(), "\n";
?>
--EXPECT--
1001
