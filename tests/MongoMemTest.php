<?php
require_once 'PHPUnit/Framework.php';

class MongoMemTest extends PHPUnit_Framework_TestCase
{
    public function testLastError2() {
      $db = $this->sharedFixture->selectDB('db');
      $mem = memory_get_usage(true);
      for ($i=0;$i<10000;$i++) {
        $db->lastError();
      }
      $this->assertEquals($mem, memory_get_usage(true));
    }
    
    public function testPrevError2() {
      $db = $this->sharedFixture->selectDB('db');
      $mem = memory_get_usage(true);
      for ($i=0;$i<10000;$i++) {
        $db->prevError();
      }
      $this->assertEquals($mem, memory_get_usage(true));
    }

    public function testResetError2() {
      $db = $this->sharedFixture->selectDB('db');
      $mem = memory_get_usage(true);
      for ($i=0;$i<10000;$i++) {
        $db->resetError();
      }
      $this->assertEquals($mem, memory_get_usage(true));
    }

    public function testForceError2() {
      $db = $this->sharedFixture->selectDB('db');
      $mem = memory_get_usage(true);
      for ($i=0;$i<10000;$i++) {
        $db->forceError();
      }
      $this->assertEquals($mem, memory_get_usage(true));
    }

    public function testSelectDB() {
      $mem = memory_get_usage(true);
      for ($i=0;$i<10000;$i++) {
        $this->sharedFixture->selectDB("foo");
      }
      $this->assertEquals($mem, memory_get_usage(true));
    }

    public function testSelectCollection() {
      $mem = memory_get_usage(true);
      for ($i=0;$i<10000;$i++) {
        $this->sharedFixture->selectCollection("foo", "bar");
      }
      $this->assertEquals($mem, memory_get_usage(true));
    }

    public function testDropDB() {
      $mem = memory_get_usage(true);
      for ($i=0;$i<10000;$i++) {
        $this->sharedFixture->dropDB("foo");
      }
      $this->assertEquals($mem, memory_get_usage(true));

      $db = $this->sharedFixture->selectDB("foo");
      for ($i=0;$i<10000;$i++) {
        $this->sharedFixture->dropDB($db);
      }
    }

    public function testGetDBRef() {
      $db = $this->sharedFixture->selectDB("foo");
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
      $c = $this->sharedFixture->selectCollection("foo", "bar");
      $obj = array("uid" => 0);
      $c->insert($obj);

      $mem = memory_get_usage(true);
      for ($i=0;$i<10000;$i++) {
        MongoDBRef::create("bar", $obj['_id']);
      }
      $this->assertEquals($mem, memory_get_usage(true));
    }

    public function testEnsureIndex() {
      $c = $this->sharedFixture->selectCollection("foo", "bar");
      $mem = memory_get_usage(true);
      for ($i=0; $i<10000; $i++) {
        $c->deleteIndexes();
      }
      $this->assertEquals($mem, memory_get_usage(true));
    }

    public function testCursorCount() {
      $c = $this->sharedFixture->selectCollection("foo", "bar");
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

