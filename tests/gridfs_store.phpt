--TEST--
mongo_gridfs - file manipulation
--FILE--
<?php 

$connection = mongo_connect("localhost:27017", "", "", false, false, false);
echo "connected\n";
$gridfs = mongo_gridfs_init($connection, "phpt", "fs.files", "fs.chunks");
echo "inited\n";
mongo_gridfs_store($gridfs, "./tests/somefile");
echo "stored\n";

?>
--EXPECTF--
connected
inited
stored
