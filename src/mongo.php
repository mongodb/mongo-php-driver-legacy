<?php

dl("libmongo.so");

class Mongo {

  var $dbname = NULL;
  var $host = "localhost";
  var $port = "27017";

  public function __construct( $_host = "localhost", $_dbname = NULL ) {
    $this->host = $_host;
    $this->db = mongo_connect( $this->host );
    $this->dbname = $_dbname;
  }

  public function __toString() {
    $str = $this->host . ":" . $this->port;
    if( $dbname != NULL )
      return $str . "/" . $this->dbname;
    return $str;
  }

  /** 
   * Sets the database to use.
   * @param string $_dbname the database name
   * @return boolean if the database name was valid
   */
  public function setDatabase( $_dbname = NULL ) {
    if( $_dbname == NULL || $_dbname == "" ) {
      trigger_error( "Invalid database name.", E_USER_WARNING );
      return false;
    }
    $this->dbname = $_dbname;
    return true;
  }

  /** 
   * Returns the name of the database currently in use.
   * @return string the name of the database
   */
  public function getDatabase() {
    return $this->dbname;
  }

  /** 
   * Gets a collection.
   * @param string $name the name of the collection
   */
  public function getCollection( $name ) {
    return new Collection( $this, $name );
  }

  /** 
   * Creates a collection.
   * @param string $name the name of the collection
   * @param bool $capped if the collection should be a fixed size
   * @param int $size if the collection is fixed size, its size in bytes
   * @param int $max if the collection is fixed size, the maximum 
   *     number of elements to store in the collection
   * @return Collection a collection object representing the new collection
   */
  public function createCollection( $name, $capped = false, $size = 0, $max = 0 ) {
    $data = array( Mongo::$CREATE_COLLECTION => $name );
    if( $capped && $size ) {
      $data[ "capped" ] = true;
      $data[ "size" ] = $size;
      if( $max )
        $data[ "max" ] = $max;
    }

    $this->dbCommand( Mongo::$CREATE_COLLECTION, $data );
    return new Collection( $this, $name );
  }

  /** 
   * Lists all of the databases.
   * @return Array each database with its size and name
   */
  public function listDatabases() {
    $data = array( Mongo::$LIST_DATABASES => 1 );
    $result = $this->dbCommand( Mongo::$LIST_DATABASES, $data );
    if( $result )
      return $result[ "databases" ];
    else
      return false;
  }

  /**
   * Closes this database connection.
   * @return bool if the connection was successfully closed
   */
  public function close() {
    mongo_close( $this->db );
  }

  public function dbCommand( $name, $data ) {
    $dbname = $this->dbname;
    // check if dbname is set
    if( $dbname == "" ) {
      // default to admin?
      $dbname = Mongo::$ADMIN;
    }

    $cmd_collection = $dbname . Mongo::$CMD;
    $obj = mongo_find_one( $this->db, $cmd_collection, $data );

    if( $obj ) {
      return $obj;
    }
    else {
      trigger_error( "no response?", E_USER_WARNING );
      return false;
    }
  }

  /* Constants */
  private static $CMD = ".\$cmd";
  /* Commands */
  private static $CREATE_COLLECTION = "create";
  private static $LIST_DATABASES = "listDatabases";

  /* Admin database */
  private static $ADMIN = "admin";
}


class Collection {

  var $collection = "";
  private $db;

  function __construct( Mongo $db, $name ) {
    $this->db = $db;
    $this->collection = $name;
  }

  /**
   * Validates this collection.
   * @return array the database's evaluation of this object
   */
  function validate() {
    $dbname = $this->db->getDatabase();
    $data = array( Collection::$VALIDATE => $this->collection );
    return $this->db->dbCommand( Collection::$VALIDATE, $data );
  }

  private static $VALIDATE = "validate";

}


?>
