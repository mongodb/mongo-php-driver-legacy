--TEST--
MongoDB::cursorCommand()
--SKIPIF--
<?php require "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require "tests/utils/server.inc";
$dsn = MongoShellServer::getStandaloneInfo();

$m = new MongoClient($dsn);
$d = $m->selectDB(dbname());
$d->cursorcmd->drop();

for ($i = 0; $i < 500; $i++) {
	$d->cursorcmd->insert( array( 'article_id' => $i ) );
}

try {
	$status = $d->cursorCommand(array(
		'foobarCommand' => 'cursorcmd', 
		'pipeline' => array( 
			array( '$match' => array() )
		)
	));
} catch (MongoResultException $e) {
	var_dump($e->getCode());
	var_dump($e->getMessage());
}
?>
--EXPECT--
int(2)
string(26) "no such cmd: foobarCommand"
