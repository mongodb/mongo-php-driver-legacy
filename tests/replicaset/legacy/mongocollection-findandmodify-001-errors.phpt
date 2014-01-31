--TEST--
MongoCollection::findAndModify() helper
--SKIPIF--
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
    echo $e->getCode(), " : ", $e->getMessage(), "\n";
    $res = $e->getDocument();
    dump_these_keys($res["lastErrorObject"], array('err', 'code', 'n', 'lastOp', 'connectionId', 'ok'));
	var_dump($res["errmsg"], $res["ok"]);
}


try {
    $retval = $col->findAndModify(
         array("inprogress" => false, "name" => "Next promo"),
         array('$pop' => array("tasks" => -1)),
         array("tasks" => array('$pop' => array("stuff"))),
         array("new" => true)
    );
} catch(MongoResultException $e) {
    echo $e->getCode(), " : ", $e->getMessage(), "\n";
    var_dump($e->getDocument());
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
    echo $e->getCode(), " : ", $e->getMessage(), "\n";
    var_dump($e->getDocument());
}

try {
    $retval = $col->findAndModify(null);
    var_dump($retval);
} catch(MongoResultException $e) {
    echo $e->getCode(), " : ", $e->getMessage(), "\n";
    $res = $e->getDocument();
    var_dump($res["errmsg"], $res["ok"]);
}

?>
--EXPECTF--
2 : %s
array(6) {
  ["err"]=>
  string(%d) "%s"
  ["code"]=>
  int(%d)
  ["n"]=>
  int(0)
  ["lastOp"]=>
  object(MongoTimestamp)#%d (2) {
    ["sec"]=>
    int(%d)
    ["inc"]=>
    int(%d)
  }
  ["connectionId"]=>
  int(%d)
  ["ok"]=>
  float(1)
}
string(%d) "%s"
float(0)
13097 : exception: Unsupported projection option: $pop
array(3) {
  ["errmsg"]=>
  string(46) "exception: Unsupported projection option: $pop"
  ["code"]=>
  int(13097)
  ["ok"]=>
  float(0)
}
12515 : exception: can't remove and update
array(3) {
  ["errmsg"]=>
  string(34) "exception: can't remove and update"
  ["code"]=>
  int(12515)
  ["ok"]=>
  float(0)
}
2 : need remove or update
string(21) "need remove or update"
float(0)
