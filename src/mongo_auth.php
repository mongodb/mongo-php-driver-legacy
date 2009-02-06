<?php

class mongo_auth {

  /**
   * Attempt to create a new authenticated session.
   * @param connection $conn a database connection
   * @param string $db the name of the db
   * @param string $username the username
   * @param string $password the password
   */
  public static function get_auth( $conn, $db, $username, $password ) {
    $db = $db . ".system.users";
    $user = mongo_find_one( $conn, $db, array( "user" => $username ) );
    if( !$user ) {
      return false;
    }
    $pwd = $user[ "pwd" ];

    $result = mongo_util::db_command( $conn, array( mongo_util::$NONCE => 1 ), $db );
    if( !$result[ "ok" ] )
      return false;
    $nonce = $result[ "nonce" ];

    $digest = md5( $nonce . $username . $pwd );
    $data = array( mongo_util::$AUTHENTICATE => 1, 
           "user" => $username, 
           "password" => $password,
           "nonce" => $nonce,
           "key" => $digest );

    $result = mongo_util::db_command( $conn, $data, $db );
    if( !$result[ "ok" ] ) {
      return false;
    }
    return new mongo_auth();
  }

  private function __construct(  ) {

  }

  public function __toString() {
    return "Authenticated";
  }

}

?>
