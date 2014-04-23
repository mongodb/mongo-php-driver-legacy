--TEST--
Test for PHP-436: MongGridFS::storeUpload() breaks on HTML5 multiple file upload.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--INI--
file_uploads=1
upload_max_filesize=1024
max_file_uploads=10
--POST_RAW--
Content-Type: multipart/form-data; boundary=---------------------------20896060251896012921717172737
-----------------------------20896060251896012921717172737
Content-Disposition: form-data; name="multifiles[]"; filename="file1.txt"
Content-Type: text/plain-file1

1
-----------------------------20896060251896012921717172737
Content-Disposition: form-data; name="multifiles[]"; filename="file2.txt"
Content-Type: text/plain-file2

2
-----------------------------20896060251896012921717172737
Content-Disposition: form-data; name="multifiles[]"; filename="file3.txt"
Content-Type: text/plain-file3

3
-----------------------------20896060251896012921717172737--
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils/server.inc";
$m = new_mongo_standalone();
$gridfs = $m->test->getGridFS();
$gridfs->drop();
try {
    $retval = $gridfs->storeUpload("multifiles");
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
array(3) {
  [0]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  [1]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  [2]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
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
    int(261120)
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
object(MongoGridFSFile)#%d (3) {
  ["file"]=>
  array(6) {
    ["_id"]=>
    object(MongoId)#%d (1) {
      ["$id"]=>
      string(24) "%s"
    }
    ["filename"]=>
    string(9) "file2.txt"
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
    int(261120)
    ["md5"]=>
    string(32) "c81e728d9d4c2f636f067f89cc14862c"
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
object(MongoGridFSFile)#%d (3) {
  ["file"]=>
  array(6) {
    ["_id"]=>
    object(MongoId)#%d (1) {
      ["$id"]=>
      string(24) "%s"
    }
    ["filename"]=>
    string(9) "file3.txt"
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
    int(261120)
    ["md5"]=>
    string(32) "eccbc87e4b5ce2fe28308fd9f2a7baf3"
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
