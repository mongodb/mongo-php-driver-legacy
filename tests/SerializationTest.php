<?php
require_once 'PHPUnit/Framework.php';

class SerializationTest extends PHPUnit_Framework_TestCase
{
    public function arrayEncode() {
      $c = $this->sharedFixture->phpunit->c->drop();

      $a = array();
      $a[-1] = 'foo';
      $c->insert($a);
      $a2 = $c->findOne();
      $this->assertEquals('foo', $a2['-1'], json_encode($a2));

      $c->remove();
      $a[-2147483647] = "bar";

      $c->insert($a);
      $a2 = $c->findOne();
      
      $this->assertEquals('bar', $a2['-2147483647'], json_encode($a2));
    }

    public function getChars($x) {
      $str = "";
      for ($i=0; $i < strlen($x); $i++) {
        $str .= ord($x[$i])." ";
      }
      return $str;
    }

    public function testNull() {
      $x = bson_encode(NULL);
      $this->assertEquals("", $x);
    }

    public function testLong() {
      $x = bson_encode(123);

      $z = chr(0);
      $s = chr(123);
      $this->assertEquals("$s$z$z$z", $x, $this->getChars($x));
    }

    public function testDouble() {
      $x = bson_encode(4.23);
      $expect = chr(236).chr(81).chr(184).chr(30).chr(133).chr(235).chr(16).chr(64);
      $this->assertEquals($expect, $x, $this->getChars($x));
    }

    public function testBool() {
      $x = bson_encode(true);
      $this->assertEquals(chr(1), $x, $this->getChars($x));
      $x = bson_encode(false);
      $this->assertEquals(chr(0), $x, $this->getChars($x));
    }

    public function testString() {
      $x = bson_encode("foofaroo");
      $this->assertEquals("foofaroo", $x);
    }

    public function testArray() {
      $x = bson_encode(array("x", 6));
      $z = chr(0);

      $str = chr(21)."$z$z$z";
      $str .= chr(2)."0$z".chr(2)."$z$z${z}x$z";
      $str .= chr(16)."1$z".chr(6)."$z$z$z";
      $str .= $z;
      $this->assertEquals($str, $x, $this->getChars($x));
    }

    /* TODO */
    public function testObj() {
      $x = bson_encode(NULL);
      $this->assertEquals("", $x);
    }

    public function testId() {
      $id = new MongoId("012345678901234567890123");
      $x = bson_encode($id);
      $str = chr(1).chr(35).chr(69).chr(103).chr(137);
      $str .= chr(1).chr(35).chr(69).chr(103).chr(137);
      $str .= chr(1).chr(35);
      $this->assertEquals($str, $x, $this->getChars($x));
    }

    /* TODO */
    public function testDate() {
      $x = bson_encode(NULL);
      $this->assertEquals("", $x);
    }

    /* TODO */
    public function testTs() {
      $x = bson_encode(NULL);
      $this->assertEquals("", $x);
    }

    /* TODO */
    public function testCode() {
      $x = bson_encode(NULL);
      $this->assertEquals("", $x);
    }

    /* TODO */
    public function testBinData() {
      $x = bson_encode(NULL);
      $this->assertEquals("", $x);
    }

    /* TODO */
    public function testRegex() {
      $x = bson_encode(NULL);
      $this->assertEquals("", $x);
    }

    /**
     * @expectedException MongoException 
     */
    public function testUpdateFree() {
      $c = $this->sharedFixture->phpunit->c;
      $c->update(array("foo" => "\xFE\xF0"), array("foo" => "\xFE\xF0"));
    }

    /**
     * @expectedException MongoException 
     */
    public function testRemoveFree() {
      $c = $this->sharedFixture->phpunit->c;
      $c->remove(array("foo" => "\xFE\xF0"));
    }

    /**
     * @expectedException MongoException 
     */
    public function testIdUTF8() {
      $c = $this->sharedFixture->phpunit->c;
      $c->insert(array("_id" => "\xFE\xF0"));
    }

    /**
     * @expectedException MongoException 
     */
    public function testCodeUTF8() {
      $code = new MongoCode("return 4;", array("x" => "\xFE\xF0"));
      $c = $this->sharedFixture->phpunit->c;
      $c->insert(array("x" => $code));
    }

    /**
     * @expectedException MongoException 
     */
    public function testClassUTF8() {
      $cls = new StdClass;
      $cls->x = "\xFE\xF0";
      $c = $this->sharedFixture->phpunit->c;
      $c->insert(array("x" => $cls));
    }

    /**
     * @expectedException MongoException 
     */
    public function testDots() {
      $c = $this->sharedFixture->phpunit->c;
      $c->insert(array("x.y" => 'yz'));
    }

    /**
     * @expectedException MongoException 
     */
    public function testEmptyKey1() {
        $c = $this->sharedFixture->phpunit->c;
        $c->save(array("" => "foo"));
    }

    /**
     * @expectedException MongoException 
     */
    public function testEmptyKey2() {
        $c = $this->sharedFixture->phpunit->c;
        $c->save(array("x" => array("" => "foo")));
    }

    /**
     * @expectedException MongoException 
     */
    public function testEmptyKey3() {
        $c = $this->sharedFixture->phpunit->c;
        $c->save(array("x" => array("" => "foo"), "y" => "z"));
    }
}
?>
