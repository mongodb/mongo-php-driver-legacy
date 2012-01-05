<?php
class MongoMemTest extends PHPUnit_Framework_TestCase
{
    public function testLastError2() {
      $m = new Mongo();
      $db = $m->selectDB('db');
      $mem = memory_get_usage(true);
      for ($i=0;$i<10000;$i++) {
        $db->lastError();
      }
      $this->assertEquals($mem, memory_get_usage(true));
    }
    
    public function testPrevError2() {
      $m = new Mongo();
      $db = $m->selectDB('db');
      $mem = memory_get_usage(true);
      for ($i=0;$i<10000;$i++) {
        $db->prevError();
      }
      $this->assertEquals($mem, memory_get_usage(true));
    }

    public function testResetError2() {
      $m = new Mongo();
      $db = $m->selectDB('db');
      $mem = memory_get_usage(true);
      for ($i=0;$i<10000;$i++) {
        $db->resetError();
      }
      $this->assertEquals($mem, memory_get_usage(true));
    }

    public function testForceError2() {
      $m = new Mongo();
      $db = $m->selectDB('db');
      $mem = memory_get_usage(true);
      for ($i=0;$i<10000;$i++) {
        $db->forceError();
      }
      $this->assertEquals($mem, memory_get_usage(true));
    }

    public function testSelectDB() {
      $m = new Mongo();
      $mem = memory_get_usage(true);
      for ($i=0;$i<10000;$i++) {
        $m->selectDB("phpunit");
      }
      $this->assertEquals($mem, memory_get_usage(true));
    }

    public function testSelectCollection() {
      $m = new Mongo();
      $mem = memory_get_usage(true);
      for ($i=0;$i<10000;$i++) {
        $m->selectCollection("phpunit", "bar");
      }
      $this->assertEquals($mem, memory_get_usage(true));
    }

    public function testDropDB() {
      $m = new Mongo();
      $mem = memory_get_usage(true);
      for ($i=0;$i<10000;$i++) {
        $m->dropDB("phpunit");
      }
      $this->assertEquals($mem, memory_get_usage(true));

      $db = $m->selectDB("phpunit");
      for ($i=0;$i<10000;$i++) {
        $m->dropDB($db);
      }
    }

    public function testGetDBRef() {
      $m = new Mongo();
      $db = $m->selectDB("phpunit");
      $c = $db->selectCollection("bar");
      $obj = array("uid" => 0);
      $c->insert($obj);
      $ref=$c->createDBRef($obj);

      $mem = memory_get_usage(true);
      for ($i=0;$i<10000;$i++) {
        MongoDBRef::get($db, $ref);
      }
      $this->assertEquals($mem, memory_get_usage(true));
    }

    public function testCreateDBRef() {
      $m = new Mongo();
      $c = $m->selectCollection("phpunit", "bar");
      $obj = array("uid" => 0);
      $c->insert($obj);

      $mem = memory_get_usage(true);
      for ($i=0;$i<10000;$i++) {
        MongoDBRef::create("bar", $obj['_id']);
      }
      $this->assertEquals($mem, memory_get_usage(true));
    }

    public function testEnsureIndex() {
      $m = new Mongo();
      $c = $m->selectCollection("phpunit", "bar");
      $mem = memory_get_usage(true);
      for ($i=0; $i<10000; $i++) {
        $c->deleteIndexes();
      }
      $this->assertEquals($mem, memory_get_usage(true));
    }

    public function testCursorCount() {
      $m = new Mongo();
      $c = $m->selectCollection("phpunit", "bar");
      $c->insert(array("foo" => "bar"));
      $c->insert(array("foo" => "bar"));
      $mem = memory_get_usage(true);
      for ($i=0; $i<10000; $i++) {
        $c->find()->count();
      }
      $this->assertEquals($mem, memory_get_usage(true));
    }
    
}

?>

