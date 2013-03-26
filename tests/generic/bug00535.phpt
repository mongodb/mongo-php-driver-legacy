--TEST--
Test for PHP-535: Run commands on Replication Secondaries
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
function dbname() {
    return "test";
}
MongoLog::setLevel(MongoLog::ALL);
MongoLog::setModule(MongoLog::ALL);
MongoLog::setCallback(function($module, $level, $msg) {
    if (strpos($msg, "command supports") !== false) {
        echo $msg, "\n";
        return;
    }
    if (strpos($msg, "forcing") !== false) {
        echo $msg, "\n";
        return;
    }
});


$mc = new MongoClient("localhost:32120");
$db = $mc->selectDB(dbname());
$collection = $db->cmd_rp;
$collection->drop();

for ($i = 0; $i<10; $i++) {
    $collection->insert(array("document" => $i));
}
/* Used for group() */
$collection->insert(array("category" => "fruit", "name" => "apple"));
$collection->insert(array("category" => "fruit", "name" => "peach"));
$collection->insert(array("category" => "fruit", "name" => "banana"));
$collection->insert(array("category" => "veggie", "name" => "corn"));
$collection->insert(array("category" => "veggie", "name" => "broccoli"));

/* Used for aggregate() */
$data = array (
    'title' => 'this is my title',
    'author' => 'bob',
    'posted' => new MongoDate,
    'pageViews' => 5,
    'tags' => array ( 'fun', 'good', 'fun' ),
    'comments' => array (
      array (
        'author' => 'joe',
        'text' => 'this is cool',
      ),
      array (
        'author' => 'sam',
        'text' => 'this is bad',
      ),
    ),
    'other' =>array (
      'foo' => 5,
    ),
);
$d = $collection->insert($data, array("w" => 1));


$collection->insert(array("user_id" => 42, 
    "time" => new MongoDate(), 
    "desc" => "some description"));




echo "Done priming data.. now the actual tests\n\n";

echo "\nTesting count\n";
var_dump($collection->count());

echo "\nTesting group\n";
$keys = array("category" => 1);
$initial = array("items" => array());
$reduce = "function (obj, prev) { prev.items.push(obj.name); }";
$retval = $collection->group($keys, $initial, $reduce);
var_dump($retval["ok"]);

echo "\nTesting dbStats\n";
$r = $db->command(array("dbStats" => 1));
var_dump($r["ok"]);

echo "\nTesting collStats\n";
$r = $db->command(array("collStats" => "cmd_rp"));
var_dump($r["ok"]);

echo "\nTesting distinct\n";
$r = $collection->distinct("category");
var_dump($r);

echo "\nTesting aggregate\n";
$ops = array(
    array(
        '$project' => array(
            "author" => 1,
            "tags"   => 1,
        )
    ),
    array('$unwind' => '$tags'),
    array(
        '$group' => array(
            "_id" => array("tags" => '$tags'),
            "authors" => array('$addToSet' => '$author'),
        ),
    ),
);
$results = $collection->aggregate($ops);
var_dump($results["ok"]);

echo "\nTesting mapreduce\n";

$map = new MongoCode("function() { emit(this.user_id,1); }");
$reduce = new MongoCode("function(k, vals) { ".
    "var sum = 0;".
    "for (var i in vals) {".
        "sum += vals[i];". 
    "}".
    "return sum; }");

$r = $db->command(array(
    "mapreduce" => "cmd_rp",
    "map" => $map,
    "reduce" => $reduce,
    "query" => array("type" => "sale"),
    "out" => array("merge" => "eventCounts")));

var_dump($r["ok"]);

echo "\nTesting *inline* mapreduce\n";
$r = $db->command(array(
    "mapreduce" => "cmd_rp",
    "map" => $map,
    "reduce" => $reduce,
    "query" => array("type" => "sale"),
    "out" => "inline"
));

var_dump($r["ok"]);
?>
--EXPECT--
forcing primary for command
forcing primary for getlasterror
forcing primary for getlasterror
forcing primary for getlasterror
forcing primary for getlasterror
forcing primary for getlasterror
forcing primary for getlasterror
forcing primary for getlasterror
forcing primary for getlasterror
forcing primary for getlasterror
forcing primary for getlasterror
forcing primary for getlasterror
forcing primary for getlasterror
forcing primary for getlasterror
forcing primary for getlasterror
forcing primary for getlasterror
forcing primary for getlasterror
forcing primary for getlasterror
Done priming data.. now the actual tests


Testing count
command supports Read Preferences
int(17)

Testing group
command supports Read Preferences
float(1)

Testing dbStats
command supports Read Preferences
float(1)

Testing collStats
command supports Read Preferences
float(1)

Testing distinct
command supports Read Preferences
array(2) {
  [0]=>
  string(5) "fruit"
  [1]=>
  string(6) "veggie"
}

Testing aggregate
command supports Read Preferences
float(1)

Testing mapreduce
forcing primary for command
float(1)

Testing *inline* mapreduce
command supports Read Preferences
float(1)

