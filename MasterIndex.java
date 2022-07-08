import java.sql.*;
import java.util.*;

public class MasterIndex{

    //global variables
    final static int NUMBER_OF_DISKS = 15;
    static Connection c = null;
    static Hashtable<String, String[]> userTable = new Hashtable<>();

    static boolean isUserInMasterList(String _id, List<MasterUser> musers){
        boolean isFound = false;
        for(MasterUser mu : musers){
            String muID = mu.getID();
            if(muID.equals(_id)){
                isFound = true;
            }
        }
        return isFound;
    }

    static Hashtable<String, String[]> getUserHashTable(List<MasterUser> mUsers){
        Hashtable<String, String[]> hash = new Hashtable<String, String[]>();

        for(MasterUser mu : mUsers){
            List<String> temp = new ArrayList<>();
            for(int i = 1; i < (NUMBER_OF_DISKS + 1); i++){
                Connection c1 = null;
                Statement stmt = null;
                String db = "";
                try{
                    Class.forName("org.sqlite.JDBC");
                    db = "sc_aepdeD";
                    String end = i < 10 ? "0" + i + ".db" : i + ".db";
                    db = db + end;
                    c1 = DriverManager.getConnection("jdbc:sqlite:" + db);
                    stmt = c1.createStatement();
                    ResultSet rs = stmt.executeQuery("SELECT ID FROM USERS WHERE id=\'" + mu.getID() + "\';");
                    String id = null;
                    while(rs.next()){
                        id = rs.getString("ID");
                    }
                    if(id != null){
                        temp.add(db);
                    }
                    rs.close();
                    stmt.close();
                }
                catch(Exception e){
                    System.err.println( e.getClass().getName() + ": " + e.getMessage() + " for " + db);
                }
            }

            String[] values = new String[temp.size()];
            for(int i = 0; i < temp.size(); i++){
                values[i] = temp.get(i);
            }
            hash.put(mu.getID(), values);
        }

        return hash;

    }

    static void createTables(){
        try {
            Statement stmt = c.createStatement();
            String sql = "DROP table IF EXISTS USERS";
            stmt.executeUpdate(sql);
            sql = "DROP table IF EXISTS DIRECTORIES";
            stmt.executeUpdate(sql);
            sql = "DROP table IF EXISTS FILES";
            stmt.executeUpdate(sql);
            sql = "CREATE TABLE USERS(" +
                    "ID TEXT PRIMARY KEY NOT NULL," +
                    "NAME       TEXT    NOT NULL," +
                    "DISKUSAGE  INT);";
            stmt.executeUpdate(sql);
            sql = "CREATE TABLE DIRECTORIES("+
            "ID     TEXT     NOT NULL,"+
            "NAME   TEXT    NOT NULL,"+
            "SIZE   INT,"+
            "FOREIGN KEY(ID) REFERENCES USERS(ID));";
            stmt.executeUpdate(sql);
            sql = "CREATE TABLE FILES("+
            "ID     TEXT     NOT NULL,"+
            "NAME   TEXT    NOT NULL,"+
            "SIZE   INT,"+
            "LASTACCESS TEXT,"+
            "FOREIGN KEY(ID) REFERENCES USERS(ID));";
            stmt.executeUpdate(sql);
        }catch (Exception e) {
            System.err.println( e.getClass().getName() + ": " + e.getMessage() );
            System.exit(0);
        }
        System.out.println("Tables created");
    }

    static List<MasterUser> indexAllUsers(){
        List<MasterUser> masterUsers = new ArrayList<MasterUser>();

        //select all users from small databases first
        for(int i = 1; i < (NUMBER_OF_DISKS + 1); i++){
            Connection c1 = null;
            Statement stmt = null;
            String db = "";
            try {
                Class.forName("org.sqlite.JDBC");
                db = "sc_aepdeD";
                String end = i < 10 ? "0" + i + ".db" : i + ".db";
                db = db + end;
                c1 = DriverManager.getConnection("jdbc:sqlite:" + db);
                stmt = c1.createStatement();
                ResultSet rs = stmt.executeQuery( "SELECT * FROM USERS;" );
                while ( rs.next() ) {
                    String id = rs.getString("ID");
                    String  name = rs.getString("NAME");
                    MasterUser mu = new MasterUser(name,id);
                    if(!isUserInMasterList(mu.getID(), masterUsers)){
                        masterUsers.add(mu);
                    }
                }
                rs.close();
                stmt.close();
                c1.close();
            } catch ( Exception e ) {
                System.err.println( e.getClass().getName() + ": " + e.getMessage() );
                System.exit(0);
            }
        }

        //now insert into master
        for(MasterUser mu : masterUsers){
            try{
                c.setAutoCommit(false);               
                String sql = "INSERT INTO USERS (ID,NAME) VALUES(?,?)";
                PreparedStatement pstmt = c.prepareStatement(sql);
                pstmt.setString(1,mu.getID());
                pstmt.setString(2,mu.getName());
                pstmt.executeUpdate();
                pstmt.close();
                c.commit();

            }catch ( Exception e ) {
                System.err.println( e.getClass().getName() + ": " + e.getMessage() );
                System.exit(0);
            }    
        }
        System.out.println("Users indexed");
        return masterUsers;
    }

    static void indexAllDirectories(){
        Enumeration<String> e = userTable.keys();
        while(e.hasMoreElements()){
            String key = e.nextElement();
            System.out.println("Indexing directories for " + key);
            String[] temp = userTable.get(key);
            for(String t : temp){
                System.out.println("Getting directories from " + t + "...");
                Connection c1 = null;
                Statement stmt = null;
                try {
                    Class.forName("org.sqlite.JDBC");
                    c1 = DriverManager.getConnection("jdbc:sqlite:" + t);
                    stmt = c1.createStatement();
                    ResultSet rs = stmt.executeQuery( "SELECT NAME FROM DIRECTORIES WHERE id=\'" + key + "\';");
                    while(rs.next()){
                        String name = rs.getString("NAME");
                        String sql = "INSERT INTO DIRECTORIES (ID,NAME) VALUES(?,?)";
                        PreparedStatement pstmt = c.prepareStatement(sql);
                        pstmt.setString(1,key);
                        pstmt.setString(2,name);
                        pstmt.executeUpdate();
                        pstmt.close();
                        c.commit();
                    }
                    rs.close();
                    stmt.close();
                    c1.close();
                } catch (Exception except) {
                    System.err.println( except.getClass().getName() + ": " + except.getMessage() );
                }
            }
        }
    }

    static void indexAllFiles (){
        Enumeration<String> e = userTable.keys();
        while(e.hasMoreElements()){
            String key = e.nextElement();
            System.out.println("Indexing files for " + key);
            String[] temp = userTable.get(key);
            for(String t : temp){
                System.out.println("Getting files from " + t + "...");
                Connection c1 = null;
                Statement stmt = null;
                try {
                    Class.forName("org.sqlite.JDBC");
                    c1 = DriverManager.getConnection("jdbc:sqlite:" + t);
                    stmt = c1.createStatement();
                    ResultSet rs = stmt.executeQuery( "SELECT NAME FROM FILES WHERE id=\'" + key + "\';");
                    c.setAutoCommit(false);
                    while( rs.next()){
                        String name = rs.getString("NAME");
                        String sql = "INSERT INTO FILES (ID,NAME) VALUES(?,?)";
                        PreparedStatement pstmt = c.prepareStatement(sql);
                        pstmt.setString(1,key);
                        pstmt.setString(2,name);
                        pstmt.executeUpdate();
                        pstmt.close();
                        c.commit();
                    }
                    stmt.close();
                    rs.close();
                    c1.close();
                } catch (Exception except) {
                    System.err.println( except.getClass().getName() + ": " + except.getMessage() );
                }   
                
            }
        }
    }

    public static void main(String[] args){
        try {
            Class.forName("org.sqlite.JDBC");
            c = DriverManager.getConnection("jdbc:sqlite:master.db");
            createTables();
            List<MasterUser> allUsers = new ArrayList<MasterUser>();
            allUsers = indexAllUsers();
            userTable = getUserHashTable(allUsers);
            indexAllDirectories();
            indexAllFiles();  
            c.commit();
            c.close();
        } catch ( Exception e ) {
            System.err.println( e.getClass().getName() + ": " + e.getMessage() );
            System.exit(0);
        }     
    }
}

class MasterUser{

    String name;
    String id;

    public MasterUser(String _name, String _id){
        this.name = _name;
        this.id = _id;
    }

    public String toString(){
        StringBuilder sb = new StringBuilder();
        sb.append("ID =\t");
        StringBuilder stb2 = new StringBuilder(this.id);
        sb.append(stb2.capacity());
        sb.append("\nNAME =\t");
        sb.append(this.name);
        return sb.toString();
    }

    public void setName(String _name){
        this.name = _name;
    }

    public void setID(String _id){
        this.id = _id;
    }

    public String getID(){
        return this.id;
    }

    public String getName(){
        return this.name;
    }
}