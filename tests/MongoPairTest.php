<?php
/**
 * Test class for Mongo paired connections.
 */
class MongoTest extends PHPUnit_Framework_TestCase
{
    /**
     * @var    Mongo
     * @access protected
     */
    protected $object;

    public function testConstruct() {
      $m = new Mongo("localhost:27017,localhost:27018", false);
      $m->pairConnect();
      $c = $m->selectCollection("phpunit", "test");
      $c->insert(array("foo", "bar"));
      
      $left = new Mongo("localhost:27017");
      $left->selectCollection("foo", "bar")->insert(array('x'=>1));
      $lerr = $left->lastError();

      $right = new Mongo("localhost:27018");
      $right->selectCollection("foo", "bar")->insert(array('x'=>1));
      $rerr = $right->lastError();
    }
}

?>

