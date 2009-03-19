<?php
/**
 *  Copyright 2009 10gen, Inc.
 * 
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *  http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * PHP version 5 
 *
 * @category Database
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @version  CVS: 000000
 * @link     http://www.mongodb.org
 */


/**
 * Gets an authenticated database connection.
 *
 * A typical usage would be:
 * <code>
 * // initial login
 * $auth_connection = new MongoAuth("mydb", "joe", "mypass");
 * if (!$auth_connection->loggedIn) {
 *    return $auth_connection->error;
 * }
 * setcookie("username", "joe");
 * setcookie("password", MongoAuth::getHash("joe", "mypass"));
 * </code>
 *
 * Then, for subsequent sessions, the cookies can be used to log in:
 * <code>
 * $username = $_COOKIE['username'];
 * $password = $_COOKIE['password'];
 * $auth_connection = new MongoAuth("mydb", $username, $password, false);
 * </code>
 *
 * @category Database
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @link     http://www.mongodb.org
 */
class MongoAuth extends Mongo
{

    public $connection;
    public $db;

    /**
     * Communicates with the database to log in a user.
     * 
     * @param connection $conn     database connection
     * @param string     $db       db name
     * @param string     $username username
     * @param string     $pwd      plaintext password
     *
     * @return array the database response
     */
    private static function _getUser($conn, $db, $username, $pwd) 
    {
        $ns = $db . ".system.users";

        // get the nonce
        $result = MongoUtil::dbCommand($conn, array(MongoUtil::$NONCE => 1 ), $db);
        if (!$result[ "ok" ]) {
            return false;
        }
        $nonce = $result[ "nonce" ];

        // create a digest of nonce/username/pwd
        $digest = md5($nonce . $username . $pwd);
        $data   = array(MongoUtil::$AUTHENTICATE => 1, 
                        "user" => $username, 
                        "nonce" => $nonce,
                        "key" => $digest);

        // send everything to the db and pray
        return MongoUtil::dbCommand($conn, $data, $db);
    }

    /**
     * Creates a hashed string from the username and password.
     * This string can be passed to MongoAuth->__construct as the password
     * with $plaintext set to false in order to login.
     *
     * @param string $username the username
     * @param string $password the password
     *
     * @return string the md5 hash of the username and password
     */
    public static function getHash($username, $password) 
    {
        return md5("${username}:mongo:${password}");
    }

    /**
     * Attempt to create a new authenticated session.
     *
     * @param string $db        the name of the db
     * @param string $username  the username
     * @param string $password  the password
     * @param string $plaintext if the password is encrypted
     * @param string $server    a database server address
     */
    public function __construct($db, 
                                $username, 
                                $password, 
                                $plaintext=true, 
                                $server=null)
    {
        parent::__construct($server);

        $this->db = $this->selectDB((string)$db);
        if ($plaintext) {
            $hash = MongoAuth::getHash($username, $password);
        } else {
            $hash = $password;
        }

        $result = MongoAuth::_getUser($this->connection, $db, $username, $hash);

        if ($result[ "ok" ] != 1) {
            $this->error    = "couldn't log in";
            $this->code     = -3;
            $this->loggedIn = false;
            return;
        }

        $this->loggedIn = true;
    }


    /**
     * Creates a new user.
     * This will not overwrite existing users, use MongoAuth::changePassword
     * to change a user's password.
     *
     * @param string $username the new user's username
     * @param string $password the new user's password
     * 
     * @return boolean if the new user was successfully created
     */
    public function addUser($username, $password) 
    {
        $c      = $this->db->selectCollection("system.users");
        $exists = $c->findOne(array("user" => $username));
        if ($exists) {
            return false;
        }
        $newUser = array("user" => $username,
                         "pwd" => MongoAuth::getHash($username, $password));
        $c->insert($newUser);
        return true;
    }



    /**
     * Changes a user's password.
     *
     * @param string $username the username
     * @param string $oldpass  the old password
     * @param string $newpass  the new password
     *
     * @return array whether the change was successful
     */
    public function changePassword($username, $oldpass, $newpass) 
    {
        $c    = $this->db->selectCollection("system.users");
        $user = $c->findOne(array("user" => $username));
        if (!$user) {
            return array("ok" => -2.0,
                         "errmsg" => "no user with username $username found");
        }
        if ($user['pwd'] == MongoAuth::getHash($username, $oldpass)) {
            $user['pwd'] = MongoAuth::getHash($username, $newpass);
            $c->update(array("user"=>$username), $user);
            return array("ok" => 1.0);
        }
        return array("ok" => -1.0,
                     "errmsg" => "incorrect old password");
    }


    /**
     * Delete a user.
     *
     * @param string $username the user to delete
     *
     * @return boolean if the user was deleted
     */
    public function deleteUser($username) 
    {
        $c = $this->db->selectCollection("system.users");
        return $c->remove(array("user" => (string)$username), true);
    }


    /**
     * Ends authenticated session.
     *
     * @return boolean if successfully ended
     */
    public function logout() 
    {
        $data   = array(MongoUtil::$LOGOUT => 1);
        $result = MongoUtil::dbCommand($this->connection, $data, $this->db);

        return $result[ "ok" ];
    }

}

?>
