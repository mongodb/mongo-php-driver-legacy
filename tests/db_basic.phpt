--TEST--
MongoDB class - basic DB functionality
--FILE--
<?php
include "Mongo.php";

$m = new Mongo();
$db = new MongoDB($m, "phpt");
echo "db: $db\n";

$db2 = $m->selectDB("phpt");
echo "db: $db\n";

$db->setProfilingLevel(MONGO_PROFILING_OFF);
$l1 = $db->getProfilingLevel();
$was1 = $db->setProfilingLevel(MONGO_PROFILING_SLOW);
$l2 = $db->getProfilingLevel();
$was2 = $db->setProfilingLevel(MONGO_PROFILING_ON);
$l3 = $db->getProfilingLevel();
$was3 = $db->setProfilingLevel(MONGO_PROFILING_OFF);
echo "$l1 $l2 $l3\n";
echo "$was1 $was2 $was3\n";


?>
--EXPECT--
db: phpt
db: phpt
0 1 2
0 1 2
