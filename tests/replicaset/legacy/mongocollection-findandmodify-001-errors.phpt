--TEST--
MongoCollection::findAndModify() helper
--SKIPIF--
<?php $needsmax = '3.0.0-RC7'; /* Causes mongod segfault on 3.0.0-RC8+ (SERVER-17387) */ ?>
<?php require_once "tests/utils/replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = new_mongo();
$col = $m->selectDB(dbname())->jobs;

$col->remove();

$col->insert(array(
     "name" => "Next promo",
     "inprogress" => false,
     "priority" => 0,
     "tasks" => array( "select product", "add inventory", "do placement"),
) );

$col->insert(array(
     "name" => "Biz report",
     "inprogress" => false,
     "priority" => 1,
     "tasks" => array( "run sales report", "email report" )
) );

$col->insert(array(
     "name" => "Biz report",
     "inprogress" => false,
     "priority" => 2,
     "tasks" => array( "run marketing report", "email report" )
    ),
    array("w" => true)
);



try {
    $retval = $col->findAndModify(
         array("inprogress" => false, "name" => "Biz report"),
         array('$set' => array('$set' => array('inprogress' => true, "started" => new MongoDate()))),
         null,
         array(
            "sort" => array("priority" => -1),
            "new" => true,
        )
    );
} catch(MongoResultException $e) {
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
    $res = $e->getDocument();
    dump_these_keys($res, array('errmsg', 'ok'));
}


try {
    $retval = $col->findAndModify(
         array("inprogress" => false, "name" => "Next promo"),
         array('$pop' => array("tasks" => -1)),
         array("tasks" => array('$pop' => array("stuff"))),
         array("new" => true)
    );
} catch(MongoResultException $e) {
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
    dump_these_keys($e->getDocument(), array('errmsg', 'code', 'ok'));
}


try {
    $retval = $col->findAndModify(null);
    var_dump($retval);
} catch(MongoResultException $e) {
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
    dump_these_keys($e->getDocument(), array('errmsg', 'ok'));
}

?>
--EXPECTF--
exception message: %s:%d: %s
exception code: %r(2|52)%r
array(2) {
  ["errmsg"]=>
  string(%d) "%s"
  ["ok"]=>
  float(0)
}
exception message: %s:%d: exception: Unsupported projection option: $pop
exception code: 13097
array(3) {
  ["errmsg"]=>
  string(46) "exception: Unsupported projection option: $pop"
  ["code"]=>
  int(13097)
  ["ok"]=>
  float(0)
}
exception message: %s:%d: need remove or update
exception code: 2
array(2) {
  ["errmsg"]=>
  string(21) "need remove or update"
  ["ok"]=>
  float(0)
}
