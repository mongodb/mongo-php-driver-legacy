--TEST--
Test for forking and connection management
--FILE--
<?php
$a = fopen("/tmp/mongo.log", "a");

function error_handler($code, $message)
{
	global $a;

	fprintf($a, "%d: %s\n", getmypid(), $message);
}

set_error_handler('error_handler');

MongoLog::setLevel(MongoLog::ALL);
MongoLog::setModule(MongoLog::ALL);

$m = new Mongo("localhost");


$col = $m->selectDb("phpunit")->fork;

$col->drop();
$col->insert(array("parent" => time()), array("safe" => 1));

$pid = pcntl_fork();
if ($pid == 0) {
	$col = $m = null;
	exit;
}

$n = 0;
while($n++ < 1000) {
	$col->insert(array("parent" => time()), array("safe" => 1));
}

echo $col->count(), "\n";;
$col = $m = null;
?>
--EXPECT--
