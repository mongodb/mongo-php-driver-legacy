<?php

define( "LOG_OFF", 0 );
define( "LOG_W", 1 );
define( "LOG_R", 2 );
define( "LOG_RW", 3 );

define( "TRACE_OFF", 0 );
define( "TRACE_SOME", 1 );
define( "TRACE_ON", 2 );

class MongoAuth {

  private $_connection;
  private $_db;

  /**
   * Attempt to create a new authenticated session.
   * @param connection $conn a database connection
   * @param string $db the name of the db
   * @param string $username the username
   * @param string $password the password
   */
  public static function getAuth( $conn, $db, $username, $password ) {
    $result = MongoAuth::getUser( $conn, $db, $username, $password );
    if( !$result[ "ok" ] ) {
      return false;
    }

    // check if we need an admin instance
    if( $db == "admin" ) {
      $auth_obj = MongoAdmin::getAuth( $conn );
    }
    else {
      $auth_obj = new MongoAuth( $conn, $db );
    }

    // return a new authenticated instance
    $auth_obj->_connection = $conn;
    $auth_obj->_db = $db;
    return $auth_obj;
  }

  private static function getUser( $conn, $db, $username, $password ) {
    $ns = $db . ".system.users";
    $user = mongo_find_one( $conn, $ns, array( "user" => $username ) );
    if( !$user ) {
      return false;
    }
    $pwd = $user[ "pwd" ];

    // get the nonce
    $result = MongoUtil::dbCommand( $conn, array( MongoUtil::$NONCE => 1 ), $db );
    if( !$result[ "ok" ] )
      return false;
    $nonce = $result[ "nonce" ];

    // create a digest of nonce/username/pwd
    $digest = md5( $nonce . $username . $pwd );
    $data = array( MongoUtil::$AUTHENTICATE => 1, 
           "user" => $username, 
           "password" => $password,
           "nonce" => $nonce,
           "key" => $digest );

    // send everything to the db and pray
    return MongoUtil::dbCommand( $conn, $data, $db );
  }

  private function __construct( $conn, $db ) {
  }

  public function __destruct() {
    $this->_connection = NULL;
    $this->_db = NULL;
  }

  public function __toString() {
    return "Authenticated";
  }

  /**
   * Ends authenticated session.
   * @return boolean if successfully ended
   */
  public function logout() {
    $data = array( MongoUtil::$LOGOUT => 1 );
    $result = MongoUtil::dbCommand( $this->_connection, $data, $this->_db );

    if( !$result[ "ok" ] ) {
      // trapped in the system forever
      return false;
    }

    $this->__destruct();
    return true;
  }
}

class MongoAdmin extends MongoAuth {

  public static function getAuth( $conn ) {
    return new MongoAdmin( $conn );
  }

  private function __construct( $conn ) {
    $this->_connection = $conn;
    $this->_db = "admin";
  }

  /** 
   * Lists all of the databases.
   * @return Array each database with its size and name
   */
  public function listDBs() {
    $data = array( MongoUtil::$LIST_DATABASES => 1 );
    $result = MongoUtil::dbCommand( $this->_connection, $data, $this->_db );
    if( $result )
      return $result[ "databases" ];
    else
      return false;
  }

  /**
   * Shuts down the database.
   * @return bool if the database was successfully shut down
   */
  public function shutdown() {
    $result = MongoUtil::dbCommand( $this->_connection, array( MongoUtil::$SHUTDOWN => 1 ), $this->_db );
    return $result[ "ok" ];
  }

  /**
   * Turns logging on/off.
   * @param int $level logging level
   * @return bool if the logging level was set
   */
  public function setLogging( $level ) {
    $result = MongoUtil::dbCommand( $this->_connection, array( MongoUtil::$LOGGING => (int)$level ), $this->_db );
    return $result[ "ok" ];
  }

  /**
   * Sets tracing level.
   * @param int $level trace level
   * @return bool if the tracing level was set
   */
  public function setTracing( $level ) {
    $result = MongoUtil::dbCommand( $this->_connection, array( MongoUtil::$TRACING => (int)$level ), $this->_db );
    return $result[ "ok" ];
  }

  /**
   * Sets only the query tracing level.
   * @param int $level trace level
   * @return bool if the tracing level was set
   */
  public function setQueryTracing( $level ) {
    $result = MongoUtil::dbCommand( $this->_connection, array( MongoUtil::$QUERY_TRACING => (int)$level ), $this->_db );
    return $result[ "ok" ];
  }

}


?>
