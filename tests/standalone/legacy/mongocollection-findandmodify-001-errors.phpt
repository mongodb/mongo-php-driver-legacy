--TEST--
MongoCollection::findAndModify() helper
--SKIPIF--
<?php require "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require "tests/utils/server.inc";

$m = mongo_standalone();
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
    dump_these_keys($res["lastErrorObject"], array('err', 'code', 'n', 'connectionId', 'ok'));
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
    $col->findAndModify(
        null,
        array("asdf"),
        null,
        array("sort" => array("priority" => -1), "remove" => true)
    );
    $retval = $col->find();
    var_dump(iterator_to_array($retval));
    $col->remove();

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
exception code: 2
array(5) {
  ["err"]=>
  string(%d) "%s"
  ["code"]=>
  int(%d)
  ["n"]=>
  int(0)
  ["connectionId"]=>
  int(%d)
  ["ok"]=>
  float(1)
}
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
exception message: %s:%d: exception: can't remove and update
exception code: 12515
array(3) {
  ["errmsg"]=>
  string(34) "exception: can't remove and update"
  ["code"]=>
  int(12515)
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
