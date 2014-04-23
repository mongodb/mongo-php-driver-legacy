--TEST--
MongoDB::command() with hash argument
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$connections = MongoClient::getConnections();

$db = $mc->selectDB(dbname());
$db->command(array('buildInfo' => 1), null, $hash);

printf("Number of connections: %d\n", count($connections));
printf("First connection hash: %s\n", $connections[0]['hash']);
printf("Command server hash: %s\n", $hash);
printf("Hashes are equal: %s\n", ($connections[0]['hash'] === $hash ? 'yes' : 'no'));

echo "\nTesting with hash argument assigned to another variable\n";

$foo = 'bar';
$hash = $foo;

$db->command(array('buildInfo' => 1), null, $hash);
printf("Original variable: %s\n", $foo);
printf("Hash variable: %s\n", $hash);

echo "\nTesting with hash argument referencing another variable\n";

$foo = 'bar';
$hash =& $foo;

$db->command(array('buildInfo' => 1), null, $hash);
printf("Original variable: %s\n", $foo);
printf("Hash variable: %s\n", $hash);

?>
===DONE===
--EXPECTF--
Number of connections: 1
First connection hash: %s:%d;-;.;%d
Command server hash: %s:%d;-;.;%d
Hashes are equal: yes

Testing with hash argument assigned to another variable
Original variable: bar
Hash variable: %s:%d;-;.;%d

Testing with hash argument referencing another variable
Original variable: %s:%d;-;.;%d
Hash variable: %s:%d;-;.;%d
===DONE===
