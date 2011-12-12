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
$grid->storeBytes($bytes, array("filename" => "demo.txt"), array('safe' => true));

// fetch it
$file = $grid->findOne(array('filename' => 'demo.txt'));

// get file descriptor
$fp = $file->getResource();

/* seek test */
$result = true;
$iter = 500;
for ($i=0; $i < $iter && $result; $i++) {
    $offset = rand(0, 2600*1024);
    $base   = rand(0, 1024* 10);

    fseek($fp, $base, SEEK_SET);
    $result &= substr($bytes, $base, 1024) === fread($fp, 1024);

    fseek($fp, $offset, SEEK_CUR);
    $result &= substr($bytes, $base + 1024 + $offset, 1024) === ($kk=fread($fp, 1024));
    
    fseek($fp, -1*$base, SEEK_END);
    $result &= substr($bytes,-1*$base, 1024) === fread($fp, 1024);
}

var_dump($result && $i === $iter);

--EXPECTF--
bool(true)
