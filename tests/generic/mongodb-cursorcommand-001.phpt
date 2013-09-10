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

$p = str_repeat("0123456789", 128 * 1024);

for ($i = 0; $i < 50; $i++) {
	$d->cursorcmd->insert( array( 'article_id' => $i, 'pad' => $p ) );
}

$cursor = $d->cursorCommand(array(
	'aggregate' => 'cursorcmd', 
	'pipeline' => array( 
		array( '$match' => array( 'article_id' => array( '$gte' => 25 ) ) )
	)
));
var_dump($cursor->info());

foreach ($cursor as $record) {
	echo $record['article_id'], "\n";
}
?>
--EXPECTF--
array(15) {
  ["ns"]=>
  string(14) "test.cursorcmd"
  ["limit"]=>
  int(0)
  ["batchSize"]=>
  int(0)
  ["skip"]=>
  int(0)
  ["flags"]=>
  int(0)
  ["query"]=>
  NULL
  ["fields"]=>
  NULL
  ["started_iterating"]=>
  bool(true)
  ["id"]=>
  int(%d)
  ["at"]=>
  int(0)
  ["numReturned"]=>
  int(0)
  ["server"]=>
  string(%d) "%s"
  ["host"]=>
  string(9) "%s"
  ["port"]=>
  int(30000)
  ["connection_type_desc"]=>
  string(10) "STANDALONE"
}
25
26
27
28
29
30
31
32
33
34
35
36
37
38
39
40
41
42
43
44
45
46
47
48
49
