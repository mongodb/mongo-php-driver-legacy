<?php

class mongo_auth {

  private $connection;
  private $db;

  /**
   * Attempt to create a new authenticated session.
   * @param connection $conn a database connection
   * @param string $db the name of the db
   * @param string $username the username
   * @param string $password the password
   */
  public static function get_auth( $conn, $db, $username, $password ) {
    $ns = $db . ".system.users";
    $user = mongo_find_one( $conn, $ns, array( "user" => $username ) );
    if( !$user ) {
      return false;
    }
    $pwd = $user[ "pwd" ];

    // get the nonce
    echo "nonce\n";
    $result = mongo_util::db_command( $conn, array( mongo_util::$NONCE => 1 ), $db );
    if( !$result[ "ok" ] )
      return false;
    $nonce = $result[ "nonce" ];

    // create a digest of nonce/username/pwd
    $digest = md5( $nonce . $username . $pwd );
    $data = array( mongo_util::$AUTHENTICATE => 1, 
           "user" => $username, 
           "password" => $password,
           "nonce" => $nonce,
           "key" => $digest );

    // send everything to the db and pray
    $result = mongo_util::db_command( $conn, $data, $db );
    if( !$result[ "ok" ] ) {
      return false;
    }

    // check if we need an admin instance
    if( $db == "admin" ) {
      $auth_obj = mongo_admin::get_auth();
    }
    else {
      $auth_obj = new mongo_auth( $conn, $db );
    }

    // return a new authenticated instance
    $auth_obj->connection = $conn;
    $auth_obj->db = $db;
    return $auth_obj;
  }

  private function __construct( $conn, $db ) {
  }

  public function __destruct() {
    $this->connection = NULL;
    $this->db = NULL;
  }

  public function __toString() {
    return "Authenticated";
  }

  /**
   * Ends authenticated session.
   * @return boolean if successfully ended
   */
  public function logout() {
    $data = array( mongo_util::$LOGOUT => 1 );
    $result = mongo_util::db_command( $this->connection, $data, $this->db );

    if( !$result[ "ok" ] ) {
      // trapped in the system forever
      return false;
    }

    $this->__destruct();
    return true;
  }
}

class mongo_admin extends mongo_auth {

  public static function get_auth() {
    return new mongo_admin();
  }

  private function __construct() {
  }

  public function shutdown() {
    $result = mongo_util::db_command( $this->connection, array( "shutdown" => 1 ), $this->db );
    return $result[ "ok" ];
  }

}

?>
