--TEST--
MongoCollection::findAndModify() helper
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require dirname(__FILE__) . "/../utils.inc";

$m = mongo();
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
    array("safe" => true)
);



$retval = $col->findAndModify(
     array("inprogress" => false, "name" => "Biz report"),
     array('$set' => array('$set' => array('inprogress' => true, "started" => new MongoDate()))),
     null,
     array(
        "sort" => array("priority" => -1),
        "new" => true,
    )
);

var_dump($retval);

$retval = $col->findAndModify(
     array("inprogress" => false, "name" => "Next promo"),
     array('$pop' => array("tasks" => -1)),
     array("tasks" => array('$pop' => array("stuff"))),
     array("new" => true)
);

var_dump($retval);


$col->findAndModify(
    null,
    array("asdf"),
    null,
    array("sort" => array("priority" => -1), "remove" => true)
);

$retval = $col->find();
var_dump(iterator_to_array($retval));
$col->remove();

$retval = $col->findAndModify(null);
var_dump($retval)

?>
--EXPECTF--
Warning: MongoCollection::findAndModify(): Modified field name may not start with $ in %s on line %d
bool(false)

Warning: MongoCollection::findAndModify(): exception: Unsupported projection option: $pop in %s on line %d
bool(false)

Warning: MongoCollection::findAndModify(): exception: can't remove and update in %s on line %d
array(3) {
  ["%s"]=>
  array(5) {
    ["_id"]=>
    object(MongoId)#6 (1) {
      ["$id"]=>
      string(24) "%s"
    }
    ["inprogress"]=>
    bool(false)
    ["name"]=>
    string(10) "Next promo"
    ["priority"]=>
    int(0)
    ["tasks"]=>
    array(2) {
      [0]=>
      string(13) "add inventory"
      [1]=>
      string(12) "do placement"
    }
  }
  ["%s"]=>
  array(5) {
    ["_id"]=>
    object(MongoId)#7 (1) {
      ["$id"]=>
      string(24) "%s"
    }
    ["name"]=>
    string(10) "Biz report"
    ["inprogress"]=>
    bool(false)
    ["priority"]=>
    int(1)
    ["tasks"]=>
    array(2) {
      [0]=>
      string(16) "run sales report"
      [1]=>
      string(12) "email report"
    }
  }
  ["%s"]=>
  array(5) {
    ["_id"]=>
    object(MongoId)#8 (1) {
      ["$id"]=>
      string(24) "%s"
    }
    ["name"]=>
    string(10) "Biz report"
    ["inprogress"]=>
    bool(false)
    ["priority"]=>
    int(2)
    ["tasks"]=>
    array(2) {
      [0]=>
      string(20) "run marketing report"
      [1]=>
      string(12) "email report"
    }
  }
}

Warning: MongoCollection::findAndModify(): need remove or update in %s on line %d
bool(false)
