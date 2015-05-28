--TEST--
Test for PHP-1426: bson_decode() buffer overflow and null-termination checks
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php

function createStringElement($name, $value, $len = null)
{
    if ($len === null) {
        // Default: string size + terminating null byte
        $len = strlen($value) + 1;
    }

    $bson  = pack('C', 2);                     // byte: field type
    $bson .= pack('a*x', $name);               // cstring: field name
    $bson .= pack('V', $len);                  // int32: string length
    $bson .= pack('a*x', $value);              // cstring: string

    return $bson;
}

function createDocument($elementList, $len = null)
{
    if ($len === null) {
        // Default: int32 size + element list size + terminating null byte
        $len = 4 + strlen($elementList) + 1;
    }

    $bson  = pack('V', $len);                  // int32: document length
    $bson .= $elementList;                     // byte*: element list
    $bson .= pack('x');                        // null byte: document terminator

    return $bson;
}

/**
 * Prints a traditional hex dump of byte values and printable characters.
 *
 * @see http://stackoverflow.com/a/4225813/162228
 * @param string $data   Binary data
 * @param integer $width Bytes displayed per line
 */
function hex_dump($data, $width = 16)
{
    static $pad = '.'; // Placeholder for non-printable characters
    static $from = '';
    static $to = '';

    if ($from === '') {
        for ($i = 0; $i <= 0xFF; $i++) {
            $from .= chr($i);
            $to .= ($i >= 0x20 && $i <= 0x7E) ? chr($i) : $pad;
        }
    }

    $hex = str_split(bin2hex($data), $width * 2);
    $chars = str_split(strtr($data, $from, $to), $width);

    $offset = 0;
    $length = $width * 3;

    foreach ($hex as $i => $line) {
        printf("%6X : %-{$length}s [%s]\n", $offset, implode(' ', str_split($line, 2)), $chars[$i]);
        $offset += $width;
    }
}

echo "Testing valid document:\n";
$bson = createDocument(createStringElement('foo', 'bar'));
hex_dump($bson);
var_dump(bson_decode($bson));


echo "\nTesting undersized element buffer (null byte read for next element type):\n";
$bson = createDocument(createStringElement('foo', 'bar', 3));
hex_dump($bson);
try {
    var_dump(bson_decode($bson));
} catch (MongoException $e) {
    printf("%s: %s\n", get_class($e), $e->getMessage());
}


echo "\nTesting undersized element buffer (non-null byte read for next element type):\n";
$bson = createDocument(createStringElement('foo', 'bar', 2));
hex_dump($bson);
try {
    var_dump(bson_decode($bson));
} catch (MongoException $e) {
    printf("%s: %s\n", get_class($e), $e->getMessage());
}


echo "\nTesting undersized document buffer (smaller than 5-bytes):\n";
$bson = createDocument(createStringElement('foo', 'bar'));
$bson = substr($bson, 0, 4);
hex_dump($bson);
try {
    var_dump(bson_decode($bson));
} catch (MongoException $e) {
    printf("%s: %s\n", get_class($e), $e->getMessage());
}


echo "\nTesting undersized document buffer (smaller than field name):\n";
$bson = createDocument(createStringElement('foo', 'bar'), 6);
hex_dump($bson);
try {
    var_dump(bson_decode($bson));
} catch (MongoException $e) {
    printf("%s: %s\n", get_class($e), $e->getMessage());
}


echo "\nTesting undersized document buffer (larger than field name):\n";
$bson = createDocument(createStringElement('foo', 'bar'), 17);
hex_dump($bson);
try {
    var_dump(bson_decode($bson));
} catch (MongoException $e) {
    printf("%s: %s\n", get_class($e), $e->getMessage());
}


echo "\nTesting oversized document buffer:\n";
$bson = createDocument(createStringElement('foo', 'bar'), 19);
hex_dump($bson);
try {
    var_dump(bson_decode($bson));
} catch (MongoException $e) {
    printf("%s: %s\n", get_class($e), $e->getMessage());
}

?>
===DONE===
<?php exit(0); ?>
--EXPECT--
Testing valid document:
     0 : 12 00 00 00 02 66 6f 6f 00 04 00 00 00 62 61 72  [.....foo.....bar]
    10 : 00 00                                            [..]
array(1) {
  ["foo"]=>
  string(3) "bar"
}

Testing undersized element buffer (null byte read for next element type):
     0 : 12 00 00 00 02 66 6f 6f 00 03 00 00 00 62 61 72  [.....foo.....bar]
    10 : 00 00                                            [..]
MongoCursorException: string for key "foo" is not null-terminated

Testing undersized element buffer (non-null byte read for next element type):
     0 : 12 00 00 00 02 66 6f 6f 00 02 00 00 00 62 61 72  [.....foo.....bar]
    10 : 00 00                                            [..]
MongoCursorException: string for key "foo" is not null-terminated

Testing undersized document buffer (smaller than 5-bytes):
     0 : 12 00 00 00                                      [....]
MongoCursorException: Reading document length would exceed buffer (4 bytes)

Testing undersized document buffer (smaller than field name):
     0 : 06 00 00 00 02 66 6f 6f 00 04 00 00 00 62 61 72  [.....foo.....bar]
    10 : 00 00                                            [..]
MongoCursorException: Reading key name for type 02 would exceed buffer

Testing undersized document buffer (larger than field name):
     0 : 11 00 00 00 02 66 6f 6f 00 04 00 00 00 62 61 72  [.....foo.....bar]
    10 : 00 00                                            [..]
MongoCursorException: Reading data for type 02 would exceed buffer for key "foo"

Testing oversized document buffer:
     0 : 13 00 00 00 02 66 6f 6f 00 04 00 00 00 62 61 72  [.....foo.....bar]
    10 : 00 00                                            [..]
MongoCursorException: Document length (19 bytes) exceeds buffer (18 bytes)
===DONE===
