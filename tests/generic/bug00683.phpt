--TEST--
Test for PHP-:
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc" ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

printLogs(MongoLog::ALL, MongoLog::ALL, "/Found option|Overwriting/");
echo "timeout only\n";
$m = new_mongo(null, true, true, array("connect" => false, "timeout" => 1));

echo "timeout and connecttimeoutms\n";
$m = new_mongo(null, true, true, array("connect" => false, "timeout" => 2, "connectTimeoutMS" => 3));

echo "connecttimeoutms only\n";
$m = new_mongo(null, true, true, array("connect" => false, "connectTimeoutMS" => 4));

echo "connecttimeoutms and timeout\n";
$m = new_mongo(null, true, true, array("connect" => false, "connectTimeoutMS" => 5, "timeout" => 6));

echo "connecttimeoutms lowercased\n";
$m = new_mongo(null, true, true, array("connect" => false, "connecttimeoutms" => 7));

?>
--EXPECT--
timeout only
- Found option 'timeout': 1
timeout and connecttimeoutms
- Found option 'timeout': 2
- Found option 'connectTimeoutMS': 3
- Overwriting previously set value for 'connectTimeoutMS' (2)
connecttimeoutms only
- Found option 'connectTimeoutMS': 4
connecttimeoutms and timeout
- Found option 'connectTimeoutMS': 5
- Found option 'timeout': 6
- Overwriting previously set value for 'connectTimeoutMS' (5)
connecttimeoutms lowercased
- Found option 'connectTimeoutMS': 7
