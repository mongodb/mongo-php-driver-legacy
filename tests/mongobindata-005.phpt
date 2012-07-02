--TEST--
MongoBinData type constants
--FILE--
<?php
echo MongoBinData::FUNC . "\n";
echo MongoBinData::BYTE_ARRAY . "\n";
echo MongoBinData::UUID . "\n";
echo MongoBinData::MD5 . "\n";
echo MongoBinData::CUSTOM . "\n";
?>
--EXPECT--
1
2
3
5
128
