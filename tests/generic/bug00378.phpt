--TEST--
Test for PHP-378: 
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) ."/../utils.inc";

$mongo = new_mongo();
$c = $mongo->selectDB('phpunit')->limit;
$c->drop();

function getAndShowInfo($r)
{
	$d = $r->getNext();
	$i = $r->info();
	if (!$d) {
		echo "EXHAUSTED\n";
	} else {
		printf("_id: %3d, v: %5d, limit: %3d, skip: %3d, batch: %3d - ",
			$d['_id'], $d['v'], $i['limit'], $i['skip'], $i['batchSize']);
		printf("numRet: %3d, at: %3d\n",
			$i['numReturned'], $i['at']);
	}
}

// insert data
for ($x = 0; $x < 250; $x++) {
	$c->insert(array('_id' => $x, 'v' => $x * $x));
}

printLogs(MongoLog::IO, MongoLog::INFO|MongoLog::WARNING);

echo "\nnormal limit by 3:\n";
$r = $c->find()->limit(3);
getAndShowInfo($r);
getAndShowInfo($r);
getAndShowInfo($r);
getAndShowInfo($r);

echo "\nnormal limit by -1:\n";
$r = $c->find()->limit(-1);
getAndShowInfo($r);
getAndShowInfo($r);

echo "\nnormal limit by -2:\n";
$r = $c->find()->limit(-2);
getAndShowInfo($r);
getAndShowInfo($r);
getAndShowInfo($r);

echo "\nnormal limit by 3, reissue after 2nd with a limit 3 (ok):\n";
$r = $c->find()->limit(3);
getAndShowInfo($r);
getAndShowInfo($r);
$r->limit(3);
getAndShowInfo($r);
getAndShowInfo($r);
getAndShowInfo($r);
getAndShowInfo($r);

echo "\nnormal limit by 3, reissue after 3rd with a limit 3 (ok):\n";
$r = $c->find()->limit(3);
getAndShowInfo($r);
getAndShowInfo($r);
getAndShowInfo($r);
$r->limit(3);
getAndShowInfo($r);
getAndShowInfo($r);
getAndShowInfo($r);
getAndShowInfo($r);

echo "\nnormal limit by 3, reissue after 2nd with a limit 0 (ok, but no more results):\n";
$r = $c->find()->limit(3);
getAndShowInfo($r);
getAndShowInfo($r);
$r->limit(0);
getAndShowInfo($r);

echo "\nnormal limit by 3, reissue after 3rd with a limit 0 (ok, but no more results):\n";
$r = $c->find()->limit(3);
getAndShowInfo($r);
getAndShowInfo($r);
getAndShowInfo($r);
$r->limit(0);
getAndShowInfo($r);

echo "\nnormal limit by 1, reissue after 1st with a limit 3 (fail):\n";
$r = $c->find()->limit(1);
getAndShowInfo($r);
try {
	$r->limit(3);
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}
getAndShowInfo($r);

echo "\nnormal limit by 3, reissue after exhausted (fail):\n";
$r = $c->find()->limit(3);
getAndShowInfo($r);
getAndShowInfo($r);
getAndShowInfo($r);
getAndShowInfo($r);
try {
	$r->limit(3);
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}

echo "\nnormal limit by -3:\n";
$r = $c->find()->limit(-3);
getAndShowInfo($r);
getAndShowInfo($r);
getAndShowInfo($r);
getAndShowInfo($r);

echo "\nnormal limit by -3, reissue after 2nd with a limit of 3 (fail):\n";
$r = $c->find()->limit(-3);
getAndShowInfo($r);
getAndShowInfo($r);
try {
	$r->limit(3);
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}
getAndShowInfo($r);
getAndShowInfo($r);
getAndShowInfo($r);

echo "\nnormal limit by -3, reissue after exhausted (fail):\n";
$r = $c->find()->limit(-3);
getAndShowInfo($r);
getAndShowInfo($r);
getAndShowInfo($r);
getAndShowInfo($r);
try {
	$r->limit(3);
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}
getAndShowInfo($r);

echo "\nnormal limit by 3, then after the 2nd fetch a limit -2 (ok, but weird):\n";
$r = $c->find()->limit(3);
getAndShowInfo($r);
getAndShowInfo($r);
try {
	$r->limit(-2);
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}
getAndShowInfo($r);
getAndShowInfo($r);
?>
--EXPECTF--
normal limit by 3:
Sending: query
_id:   0, v:     0, limit:   3, skip:   0, batch:   0 - numRet:   3, at:   1
_id:   1, v:     1, limit:   3, skip:   0, batch:   0 - numRet:   3, at:   2
_id:   2, v:     4, limit:   3, skip:   0, batch:   0 - numRet:   3, at:   3
Killing unfinished cursor %d
EXHAUSTED

normal limit by -1:
Sending: query
_id:   0, v:     0, limit:  -1, skip:   0, batch:   0 - numRet:   1, at:   1
EXHAUSTED

normal limit by -2:
Sending: query
_id:   0, v:     0, limit:  -2, skip:   0, batch:   0 - numRet:   2, at:   1
_id:   1, v:     1, limit:  -2, skip:   0, batch:   0 - numRet:   2, at:   2
EXHAUSTED

normal limit by 3, reissue after 2nd with a limit 3 (ok):
Sending: query
_id:   0, v:     0, limit:   3, skip:   0, batch:   0 - numRet:   3, at:   1
_id:   1, v:     1, limit:   3, skip:   0, batch:   0 - numRet:   3, at:   2
_id:   2, v:     4, limit:   5, skip:   0, batch:   0 - numRet:   3, at:   3
Sending: Get more data
_id:   3, v:     9, limit:   5, skip:   0, batch:   0 - numRet:   5, at:   4
_id:   4, v:    16, limit:   5, skip:   0, batch:   0 - numRet:   5, at:   5
Killing unfinished cursor %d
EXHAUSTED

normal limit by 3, reissue after 3rd with a limit 3 (ok):
Sending: query
_id:   0, v:     0, limit:   3, skip:   0, batch:   0 - numRet:   3, at:   1
_id:   1, v:     1, limit:   3, skip:   0, batch:   0 - numRet:   3, at:   2
_id:   2, v:     4, limit:   3, skip:   0, batch:   0 - numRet:   3, at:   3
Sending: Get more data
_id:   3, v:     9, limit:   6, skip:   0, batch:   0 - numRet:   6, at:   4
_id:   4, v:    16, limit:   6, skip:   0, batch:   0 - numRet:   6, at:   5
_id:   5, v:    25, limit:   6, skip:   0, batch:   0 - numRet:   6, at:   6
Killing unfinished cursor %d
EXHAUSTED

normal limit by 3, reissue after 2nd with a limit 0 (ok, but no more results):
Sending: query
_id:   0, v:     0, limit:   3, skip:   0, batch:   0 - numRet:   3, at:   1
_id:   1, v:     1, limit:   3, skip:   0, batch:   0 - numRet:   3, at:   2
Killing unfinished cursor %d
EXHAUSTED

normal limit by 3, reissue after 3rd with a limit 0 (ok, but no more results):
Sending: query
_id:   0, v:     0, limit:   3, skip:   0, batch:   0 - numRet:   3, at:   1
_id:   1, v:     1, limit:   3, skip:   0, batch:   0 - numRet:   3, at:   2
_id:   2, v:     4, limit:   3, skip:   0, batch:   0 - numRet:   3, at:   3
Killing unfinished cursor %d
EXHAUSTED

normal limit by 1, reissue after 1st with a limit 3 (fail):
Sending: query
_id:   0, v:     0, limit:   1, skip:   0, batch:   0 - numRet:   1, at:   1
Cannot modify limit after cursor has been exhausted.
EXHAUSTED

normal limit by 3, reissue after exhausted (fail):
Sending: query
_id:   0, v:     0, limit:   3, skip:   0, batch:   0 - numRet:   3, at:   1
_id:   1, v:     1, limit:   3, skip:   0, batch:   0 - numRet:   3, at:   2
_id:   2, v:     4, limit:   3, skip:   0, batch:   0 - numRet:   3, at:   3
Killing unfinished cursor %d
EXHAUSTED
Cannot modify limit after cursor has been exhausted.

normal limit by -3:
Sending: query
_id:   0, v:     0, limit:  -3, skip:   0, batch:   0 - numRet:   3, at:   1
_id:   1, v:     1, limit:  -3, skip:   0, batch:   0 - numRet:   3, at:   2
_id:   2, v:     4, limit:  -3, skip:   0, batch:   0 - numRet:   3, at:   3
EXHAUSTED

normal limit by -3, reissue after 2nd with a limit of 3 (fail):
Sending: query
_id:   0, v:     0, limit:  -3, skip:   0, batch:   0 - numRet:   3, at:   1
_id:   1, v:     1, limit:  -3, skip:   0, batch:   0 - numRet:   3, at:   2
Cannot modify limit after cursor has been exhausted.
_id:   2, v:     4, limit:  -3, skip:   0, batch:   0 - numRet:   3, at:   3
EXHAUSTED
EXHAUSTED

normal limit by -3, reissue after exhausted (fail):
Sending: query
_id:   0, v:     0, limit:  -3, skip:   0, batch:   0 - numRet:   3, at:   1
_id:   1, v:     1, limit:  -3, skip:   0, batch:   0 - numRet:   3, at:   2
_id:   2, v:     4, limit:  -3, skip:   0, batch:   0 - numRet:   3, at:   3
EXHAUSTED
Cannot modify limit after cursor has been exhausted.
EXHAUSTED

normal limit by 3, then after the 2nd fetch a limit -2 (ok, but weird):
Sending: query
_id:   0, v:     0, limit:   3, skip:   0, batch:   0 - numRet:   3, at:   1
_id:   1, v:     1, limit:   3, skip:   0, batch:   0 - numRet:   3, at:   2
Cannot set a negative limit after the cursor started iterating.
_id:   2, v:     4, limit:   3, skip:   0, batch:   0 - numRet:   3, at:   3
Killing unfinished cursor %d
EXHAUSTED
