--TEST--
Test for PHP-1402: hasNext/getNext iteration with various limit/skip combinations
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);

$c = $m->selectCollection(dbname(), collname(__FILE__));
$c->drop();

$c->save(array('_id' => 1));
$c->save(array('_id' => 2));
$c->save(array('_id' => 3));

foreach (range(-1, 3) as $limit) {
    foreach (range(0, 3) as $skip) {
        printf("Iterating (limit: %d, skip: %d)\n", $limit, $skip);

        $cur = $c->find()->limit($limit)->skip($skip);

        while ($cur->hasNext()) {
            $r = $cur->getNext();
            echo $r['_id'], ' ';
        }

        echo "\n\n";
    }
}

?>
===DONE===
--EXPECT--
Iterating (limit: -1, skip: 0)
1 

Iterating (limit: -1, skip: 1)
2 

Iterating (limit: -1, skip: 2)
3 

Iterating (limit: -1, skip: 3)


Iterating (limit: 0, skip: 0)
1 2 3 

Iterating (limit: 0, skip: 1)
2 3 

Iterating (limit: 0, skip: 2)
3 

Iterating (limit: 0, skip: 3)


Iterating (limit: 1, skip: 0)
1 

Iterating (limit: 1, skip: 1)
2 

Iterating (limit: 1, skip: 2)
3 

Iterating (limit: 1, skip: 3)


Iterating (limit: 2, skip: 0)
1 2 

Iterating (limit: 2, skip: 1)
2 3 

Iterating (limit: 2, skip: 2)
3 

Iterating (limit: 2, skip: 3)


Iterating (limit: 3, skip: 0)
1 2 3 

Iterating (limit: 3, skip: 1)
2 3 

Iterating (limit: 3, skip: 2)
3 

Iterating (limit: 3, skip: 3)


===DONE===
