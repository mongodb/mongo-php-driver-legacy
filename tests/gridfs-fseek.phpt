--TEST--
Testing fseek and fread
--FILE--
<?php
$conn = new Mongo();
$db   = $conn->selectDb('admin');
$grid = $db->getGridFs('wrapper');

// delete any previous results
$grid->drop();

// dummy file
$bytes = "";
for ($i=0; $i < 200*1024; $i++) {
    $bytes .= sha1(rand(1, 1000000000));
}
$length = 200*1024 * 40;

$grid->storeBytes($bytes, array("filename" => "demo.txt"));

// fetch it
$file = $grid->findOne(array('filename' => 'demo.txt'));
$chunkSize = $file->file['chunkSize'];

// get file descriptor
$fp = $file->getResource();

/* seek test */
$result = true;
$iter   = 5000;
for ($i=0; $i < $iter && $result; $i++) {
    $offset = rand(0, $chunkSize/2);
    $base   = rand(0, $chunkSize/2);

    fseek($fp, $base, SEEK_SET);
    $result &= ((string)substr($bytes, $base, 1024)) === ($read=fread($fp, 1024));
    if (!$result) {
        var_dump($offset, $base, $read);
        die("FAILED: SEEK_SET");
    }

    fseek($fp, $offset, SEEK_CUR);
    $result &= ((string)substr($bytes, $base + 1024 + $offset, 1024)) === ($read=fread($fp, 1024));
    if (!$result) {
        var_dump($offset, $base, $read);
        die("FAILED: SEEK_CUR");
    }
    
    fseek($fp, -1*$base, SEEK_END);
    $result &= ((string)substr($bytes, $length - $base, 1024)) === ($read=fread($fp, 1024));
    if (!$result) {
        var_dump($offset, $base, $read);
        die("FAILED: SEEK_END");
    }
}

var_dump($result && $i === $iter);

--EXPECTF--
bool(true)
