--TEST--
MongoCollection::findAndModify() helper
--SKIPIF--
<?php require "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require "tests/utils/server.inc";
function dumpDoc($doc) {
    /* Make the dumped doc consistent as MongoDB sometimes reorders the fields */
    $dup = array();
    foreach(array("_id", "inprogress", "name", "priority", "started", "tasks") as $k) {
        if (isset($doc[$k])) {
            $dup[$k] = $doc[$k];
        }
    }
    var_dump($dup);
}

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



$retval = $col->findAndModify(
     array("inprogress" => false, "name" => "Biz report"),
     array('$set' => array('inprogress' => true, "started" => new MongoDate())),
     null,
     array(
        "sort" => array("priority" => -1),
        "new" => true,
    )
);

dumpDoc($retval);

$retval = $col->findAndModify(
     array("inprogress" => false, "name" => "Next promo"),
     array('$pop' => array("tasks" => -1)),
     array("tasks" => 1),
     array("new" => false)
);

dumpDoc($retval);


$col->findAndModify(
    null,
    null,
    null,
    array("sort" => array("priority" => -1), "remove" => true)
);

$retval = $col->find();
foreach($retval as $ret) {
    dumpDoc($ret);
}
$col->remove();

try {
    $retval = $col->findAndModify(null);
    var_dump($retval);
} catch(MongoResultException $e) {
    echo $e->getCode(), " ", $e->getMessage(), "\n";
    $err = $e->getDocument();
    var_dump($err["errmsg"], $err["ok"]);
}

?>
--EXPECTF--
array(6) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["inprogress"]=>
  bool(true)
  ["name"]=>
  string(10) "Biz report"
  ["priority"]=>
  int(2)
  ["started"]=>
  object(MongoDate)#%d (2) {
    ["sec"]=>
    int(%d)
    ["usec"]=>
    int(%d)
  }
  ["tasks"]=>
  array(2) {
    [0]=>
    string(20) "run marketing report"
    [1]=>
    string(12) "email report"
  }
}
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["tasks"]=>
  array(3) {
    [0]=>
    string(14) "select product"
    [1]=>
    string(13) "add inventory"
    [2]=>
    string(12) "do placement"
  }
}
array(5) {
  ["_id"]=>
  object(MongoId)#%d (1) {
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
array(5) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["inprogress"]=>
  bool(false)
  ["name"]=>
  string(10) "Biz report"
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
2 need remove or update
string(21) "need remove or update"
float(0)
