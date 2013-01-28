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
		var_dump($res["lastErrorObject"], $res["errmsg"], $res["ok"]);
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
0 : Modified field name may not start with $
array(5) {
	["err"]=>
	string(40) "Modified field name may not start with $"
	["code"]=>
	int(15896)
	["n"]=>
	int(0)
	["connectionId"]=>
	int(%d)
	["ok"]=>
	float(1)
}
string(40) "Modified field name may not start with $"
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
0 : need remove or update
string(21) "need remove or update"
float(0)
