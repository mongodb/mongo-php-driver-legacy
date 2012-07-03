--TEST--
MongoRegex::__toString()
--FILE--
<?php
$regex = new MongoRegex('/foo[bar]{3}/imx');
echo (string) $regex . "\n";
?>
--EXPECT--
/foo[bar]{3}/imx
