--TEST--
Test for PHP-683: Add support for the connectTimeoutMS connection parameter.
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc" ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

printLogs(MongoLog::ALL, MongoLog::ALL, "/(Found option.*imeout*)|Replacing/");
echo "timeout only\n";
$m = new_mongo(null, true, true, array("connect" => false, "timeout" => 1));

echo "timeout and connectTimeoutMS\n";
$m = new_mongo(null, true, true, array("connect" => false, "timeout" => 2, "connectTimeoutMS" => 3));

echo "connectTimeoutMS only\n";
$m = new_mongo(null, true, true, array("connect" => false, "connectTimeoutMS" => 4));

echo "connectTimeoutMS and timeout\n";
$m = new_mongo(null, true, true, array("connect" => false, "connectTimeoutMS" => 5, "timeout" => 6));

echo "connecttimeoutms lowercased\n";
$m = new_mongo(null, true, true, array("connect" => false, "connecttimeoutms" => 7));

?>
--EXPECT--
timeout only
- Found option 'timeout' ('connectTimeoutMS'): 1
timeout and connectTimeoutMS
- Found option 'timeout' ('connectTimeoutMS'): 2
- Replacing previously set value for 'connectTimeoutMS' (2)
- Found option 'connectTimeoutMS': 3
connectTimeoutMS only
- Found option 'connectTimeoutMS': 4
connectTimeoutMS and timeout
- Found option 'connectTimeoutMS': 5
- Replacing previously set value for 'connectTimeoutMS' (5)
- Found option 'timeout' ('connectTimeoutMS'): 6
connecttimeoutms lowercased
- Found option 'connectTimeoutMS': 7
