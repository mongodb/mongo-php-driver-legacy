--TEST--
Test for PHP-535: Run commands on Replication Secondaries
--SKIPIF--
<?php if (version_compare(phpversion(), "5.3.0", "lt")) exit("skip setCallback and closures are 5.3+"); ?>
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

MongoLog::setLevel(MongoLog::ALL);
MongoLog::setModule(MongoLog::ALL);
MongoLog::setCallback(function($module, $level, $msg) {
    if (strpos($msg, "command supports") !== false) {
        echo $msg, "\n";
        return;
    }
    if (strpos($msg, "forcing") !== false) {
        if (strpos($msg, "ommand") !== false) {
            echo $msg, "\n";
            return;
        }
    }
});

$rs = MongoShellServer::getReplicasetInfo();
$mc = new MongoClient($rs['dsn'], array('replicaSet' => $rs['rsname']));

$db = $mc->selectDB(dbname());
$collection = $db->selectCollection('bug535');
$collection->drop();

$collection->insert(array('category' => 'fruit', 'name' => 'apple'));
$collection->insert(array('category' => 'fruit', 'name' => 'peach'));
$collection->insert(array('category' => 'fruit', 'name' => 'banana'));
$collection->insert(array('category' => 'veggie', 'name' => 'corn'));
$collection->insert(array('category' => 'veggie', 'name' => 'broccoli'));

echo "\nDone priming data... now the actual tests\n";

echo "\nTesting count\n";
var_dump($collection->count());

echo "\nTesting group\n";
$result = $collection->group(
    array('category' => 1),
    array('items' => array()),
    new MongoCode('function (obj, prev) { prev.items.push(obj.name); }')
);
var_dump($result['ok']);

echo "\nTesting dbStats\n";
$result = $db->command(array('dbStats' => 1));
var_dump($result['ok']);

echo "\nTesting collStats\n";
$result = $db->command(array('collStats' => 'bug535'));
var_dump($result['ok']);

echo "\nTesting distinct\n";
$result = $collection->distinct('category');
var_dump($result);

echo "\nTesting aggregate\n";
$result = $collection->aggregate(array('$match' => array('category' => 'fruit')));
var_dump($result["ok"]);

echo "\nTesting mapReduce\n";
$map = new MongoCode('function() { emit(this.category, 1); }');
$reduce = new MongoCode('function(k, vals) { var sum = 0; for (var i in vals) { sum += vals[i]; } return sum; }');
$result = $db->command(array(
    'mapReduce' => 'bug535',
    'map' => $map,
    'reduce' => $reduce,
    'out' => array('replace' => 'bug535.mapReduce'),
));
var_dump($result['ok']);

echo "\nTesting *inline* mapReduce\n";
$result = $db->command(array(
    'mapReduce' => 'bug535',
    'map' => $map,
    'reduce' => $reduce,
    'out' => array('inline' => 1),
));
var_dump($result["ok"]);

?>
--EXPECT--
forcing primary for command

Done priming data... now the actual tests

Testing count
command supports Read Preferences
int(5)

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

Testing mapReduce
forcing primary for command
float(1)

Testing *inline* mapReduce
command supports Read Preferences
float(1)
