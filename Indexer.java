import java.sql.*;
import java.util.ArrayList;

public class Indexer{
    static Connection c = null;

    static void findFile(String[] fileNames, int maxDisplay, boolean force){
        Statement stmt = null;
        for(int i = 0; i < fileNames.length; i++){
            try {
                stmt = c.createStatement();
                ResultSet rs = stmt.executeQuery( "SELECT NAME FROM FILES WHERE name like \'%" + fileNames[i] + "%\';");
                while( rs.next()){
                    String name = rs.getString("NAME");
                    System.out.println(name);
                }
                stmt.close();
                rs.close();
            } catch (Exception except) {
                System.err.println( except.getClass().getName() + ": " + except.getMessage());
                System.exit(0);
            }
        }
    }

    static void printHelp(){
        System.out.println("Usage: indexer [-H, --help] [-f, --file[...]]\n");

        System.out.println("Positional arguments:");
        System.out.println("-f, --file\tGiven a file name (or list of file names) search the database and match any files with given sequence of characters");

        System.out.println("\nPlease report bugs to nicholas.facciola@intel.com");
    }

    public static void main(String[] args){
        try{
            Class.forName("org.sqlite.JDBC");
            c = DriverManager.getConnection("jdbc:sqlite:test.db");
        }catch (Exception e){
            System.err.println(e.getClass().getName() + ": " + e.getMessage());
            System.exit(0);
        }

        switch(args[0]){
            case "-f":
            case "--file":
                if(args.length < 2){
                    System.err.println("Error... please specify a file name to search for");
                    System.exit(0);
                }else{
                    ArrayList<String> files = new ArrayList<>();
                    for(int i = 1; i < args.length; i++){
                        if(args[i].charAt(0) != '-'){
                            files.add(args[i]);
                        }
                    }
                    String[] filesToSearch = new String[files.size()];
                    for(int i = 0; i < filesToSearch.length; i++){
                        filesToSearch[i] = files.get(i);
                    }
                    findFile(filesToSearch, 50, false);
                }
            break;
            case "-h":
            case "--help":
                printHelp();
                break;
            default:
                System.out.println("Unrecognized argument...\n");
                printHelp();
                break;
        }
    }
}