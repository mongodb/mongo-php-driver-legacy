<?php

class MongoUtil {
  
  /**
   * Turns something into an array that can be saved to the db.
   * Returns the empty array if passed NULL.
   * @param any $obj object to convert
   * @return array the array
   */
  public static function objToArray( $obj ) 
  {
    if (is_null($obj)) {
      return array();
    }

    $arr = array();
    foreach ($obj as $key => $value) {
      if (is_object($value) || is_array($value)) {
        $arr[ $key ] = MongoUtil::objToArray( $value );
      } else {
        $arr[ $key ] = $value;
      }
    }
    return $arr;
  }

  /**
   * Converts a field or array of fields into an underscore-separated string.
   * @param string|array $keys field(s) to convert
   * @return string the index name
   */
  public static function toIndexString( $keys ) {
    if( is_string( $keys ) ) {
      $name = str_replace( ".", "_", $keys ) + "_1";
    }
    else {
      $key_list = array();
      foreach( $keys as $v ) {
        $key_list[] = str_replace( ".", "_", $v ) + "_1";
      }
      $name = implode( "_", $key_list );
    }
    return $name;
  }

  /**
   * Returns a string representation of an object.
   * @param array $obj the object to transform
   * @return string the string
   */
  public static function toJSON( $obj ) {
    if( is_array( $obj ) ) {
      $str = "array( ";
      foreach( $obj as $k=>$v ) {
        $str .= "\"$k\" => ";
        if( is_array( $v ) ) {
          $str .= MongoUtil::toJSON( $v );
        }
        else if( is_string( $v ) ) {
          $str .= "\"$v\"";
        }
        else {
          $str .= "$v";
        }
        if( next( $obj ) ) {
          $str .= ",";
        }
      }
      $str .= " )";
    }
    else {
      return "$obj";
    }
    return $str;
  }

  /** Execute a db command
   * @param array $data the query to send
   * @param string $db the database name
   */
  public static function dbCommand( $conn, $data, $db ) {
    $cmd_collection = $db . MongoUtil::$CMD;
    $obj = mongo_find_one( $conn, $cmd_collection, $data );

    if( $obj ) {
      return $obj;
    }
    else {
      trigger_error( "no response?", E_USER_WARNING );
      return false;
    }
  }

  /**
   * Parse boolean configuration settings from php.ini.
   * @param string $str the setting name
   * @return bool the value of the setting
   */
  public static function getConfig( $str ) {
    $setting = get_cfg_var( $str );
    if( !$setting || strcasecmp( $setting, "off" ) == 0 ) {
      return false;
    }
    return true;
  }

  /* Command collection */
  private static $CMD = ".\$cmd";

  /* Admin database */
  public static $ADMIN = "admin";

  /* Commands */
  public static $AUTHENTICATE = "authenticate";
  public static $CREATE_COLLECTION = "create";
  public static $DELETE_INDICES = "deleteIndexes";
  public static $DROP = "drop";
  public static $DROP_DATABASE = "dropDatabase";
  public static $LAST_ERROR = "getlasterror";
  public static $LIST_DATABASES = "listDatabases";
  public static $LOGGING = "opLogging";
  public static $LOGOUT = "logout";
  public static $NONCE = "getnonce";
  public static $PREV_ERROR = "getpreverror";
  public static $PROFILE = "profile";
  public static $QUERY_TRACING = "queryTraceLevel";
  public static $REPAIR_DATABASE = "repairDatabase";
  public static $RESET_ERROR = "reseterror";
  public static $SHUTDOWN = "shutdown";
  public static $TRACING = "traceAll";
  public static $VALIDATE = "validate";

}

define( "MONGO_LT", "\$lt" );
define( "MONGO_LTE", "\$lte" );
define( "MONGO_GT", "\$gt" );
define( "MONGO_GTE", "\$gte" );
define( "MONGO_IN", "\$in" );
define( "MONGO_NE", "\$ne" );

?>
