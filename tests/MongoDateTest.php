<?php
require_once 'PHPUnit/Framework.php';

/**
 * Test class for MongoDate
 */
class MongoDateTest extends PHPUnit_Framework_TestCase
{
    public function testBasic() {
        $t = time();

        $d1 = new MongoDate();
        $d2 = new MongoDate($d1->sec, $d1->usec);
        $this->assertEquals($d1->sec, $d2->sec);
        $this->assertEquals($d1->usec, $d2->usec);
        $this->assertEquals($t, $d1->sec);

        $d3 = new MongoDate(strtotime('2009-05-01 00:00:00'));
        $this->assertEquals(1241150400, $d3->sec);
        $this->assertEquals(0, $d3->usec);
    }

    public function testMilliseconds() {
        $d = new MongoDate();
        $this->assertEquals($d->usec, ($d->usec/1000)*1000);
    }

    public function testCompare() {
      $x = array(0,1,2,3,4);
      $y = array();

      for ($i=4; $i>=0; $i--) {
        $x[$i] = new MongoDate();
        $y[] = $x[$i];
        sleep(1);
      }

      sort($x);

      $this->assertEquals($x[0], $y[0]);
      $this->assertEquals($x[1], $y[1]);
      $this->assertEquals($x[2], $y[2]);
      $this->assertEquals($x[3], $y[3]);
      $this->assertEquals($x[4], $y[4]);
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInvalidParam1() {
      $object = array( 'created' => '1008597415', ); 
      $object['created'] = new MongoDate($object['created']); 
    }

    /**
     * @expectedException PHPUnit_Framework_Error
     */
    public function testInvalidParam2() {
      $object = array( 'created' => '1008597415', ); 
      $object['created'] = new MongoDate(0, $object['created']); 
    }
}

?>
