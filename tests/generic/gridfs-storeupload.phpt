--TEST--
MongGridFS::storeUpload() uploading one file
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--INI--
file_uploads=1
upload_max_filesize=1024
max_file_uploads=10
--POST_RAW--
Content-Type: multipart/form-data; boundary=---------------------------20896060251896012921717172737
-----------------------------20896060251896012921717172737
Content-Disposition: form-data; name="file1"; filename="file1.txt"
Content-Type: text/plain-file1

1
-----------------------------20896060251896012921717172737
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils/server.inc";

$m = new_mongo_standalone();
$gridfs = $m->selectDB(dbname())->getGridFS();
$gridfs->remove();
try {
    $retval = $gridfs->storeUpload("file1");
    var_dump($retval);
} catch(Exception $e) {
    var_dump($e->getMessage());
}
foreach($gridfs->find() as $file) {
    var_dump($file);
    $gridfs->delete($file->file["_id"]);
}
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
object(MongoId)#%d (1) {
  ["$id"]=>
  string(24) "%s"
}
object(MongoGridFSFile)#%d (3) {
  ["file"]=>
  array(6) {
    ["_id"]=>
    object(MongoId)#%d (1) {
      ["$id"]=>
      string(24) "%s"
    }
    ["filename"]=>
    string(9) "file1.txt"
    ["uploadDate"]=>
    object(MongoDate)#%d (2) {
      ["sec"]=>
      int(%d)
      ["usec"]=>
      int(%d)
    }
    ["length"]=>
    int(1)
    ["chunkSize"]=>
    int(262144)
    ["md5"]=>
    string(32) "c4ca4238a0b923820dcc509a6f75849b"
  }
  ["gridfs":protected]=>
  object(MongoGridFS)#%d (5) {
    ["w"]=>
    int(1)
    ["wtimeout"]=>
    int(10000)
    ["chunks"]=>
    object(MongoCollection)#%d (2) {
      ["w"]=>
      int(1)
      ["wtimeout"]=>
      int(10000)
    }
    ["filesName":protected]=>
    string(8) "fs.files"
    ["chunksName":protected]=>
    string(9) "fs.chunks"
  }
  ["flags"]=>
  int(0)
}
===DONE===
