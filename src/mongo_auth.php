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
    echo "login\n";
    $result = mongo_util::db_command( $conn, $data, $db );
    if( !$result[ "ok" ] ) {
      return false;
    }

    // create a new authenticated instance
    return new mongo_auth( $conn, $db );
  }

  private function __construct( $conn, $db ) {
    $this->connection = $conn;
    $this->db = $db;
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
    $result = mongo_util::db_command( $this->connection, $data, $db );

    if( !$result[ "ok" ] ) {
      // trapped in the system forever
      return false;
    }

    $this->__destruct();
    return true;
  }
}

?>
