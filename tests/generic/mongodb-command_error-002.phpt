--TEST--
MongoDB::command() with cursor option
--SKIPIF--
<?php require "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require "tests/utils/server.inc";
$dsn = MongoShellServer::getStandaloneInfo();

$m = new MongoClient($dsn);
$d = $m->selectDB(dbname());

try {
	$retval = $d->command(array( 'buildInfo' => 1, 'cursor' => array('batchSize' => 1 ) ) );
}
catch (MongoException $e)
{
	var_dump($e->getCode());
	var_dump($e->getMessage());
}
?>
--EXPECTF--
int(22)
string(%d) "You can't ask for a cursor with 'command()' use 'cursorCommand()'"
