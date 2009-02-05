<?php

dl("libmongo.so");

class Mongo {

  var $dbname = "";
  var $host = "localhost";
  var $port = "27017";

  public function __construct( $_host = "localhost", $_dbname = "" ) {
    $this->host = $_host;
    $this->db = mongo_connect( $this->host );
    $this->dbname = $_dbname;
  }

  public function setDatabase( $_dbname = "" ) {
    if( $_dbname == "" ) {
      trigger_error( "Invalid database name.", E_USER_WARNING );
      return false;
    }
    $this->dbname = $_dbname;
  }

  public function getDatabase() {
    return $this->dbname;
  }

  public function listDatabases() {
    return $this->dbCommand( Mongo::$LIST_DATABASES );
  }

  public function close() {
    mongo_close( $this->db );
  }

  private function dbCommand( $name ) {
    $dbname = $this->dbname;
    // check if dbname is set
    if( $dbname == "" ) {
      // default to admin?
      $dbname = Mongo::$ADMIN;
    }

    $obj = mongo_find_one( $this->db, $dbname . Mongo::$CMD, array( $name => 1 ) );
    if( $obj ) {
      return $obj;
    }
    else {
      trigger_error( "no response?", E_WARNING );
      return false;
    }
  }

  /* Constants */
  private static $CMD = ".\$cmd";
  /* Commands */
  private static $LIST_DATABASES = "listDatabases";

  /* Admin database */
  private static $ADMIN = "admin";
}


?>