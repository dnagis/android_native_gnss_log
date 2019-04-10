#include "header.h"
#include <cutils/klog.h>
#include <android-base/file.h> //libbase @ LOCAL_SHARED_LIBRARIES
#include <sqlite3.h> //libsqlite @ LOCAL_SHARED_LIBRARIES 
#include <fstream>

#define LOG_TAG "gpsvvnx"
#define KLOG_LEVEL 6


using namespace android;

void action() {

	timespec ts;
	sqlite3 *db;
	int rc;		
	std::string stmt, charge, temp;
	
	
	clock_gettime(CLOCK_REALTIME, &ts);	    
	
	std::ifstream ifile_c("/sys/class/power_supply/battery/charge_counter");
    ifile_c >> charge; 
    ifile_c.close();
    
    std::ifstream ifile_t("/sys/class/power_supply/battery/temp");
    ifile_t >> temp; 
    ifile_t.close();
    
	KLOG_WARNING(LOG_TAG, "********Timer Triggered****** @ %ld charge %s temp %s\n", (long)ts.tv_sec, charge.c_str(), temp.c_str());
	
	//build string stmt à passer à sqlite
	stmt = "insert into temp values(NULL,";
	stmt += std::to_string(ts.tv_sec);
	stmt +=  ",";
	stmt += charge.c_str();
	stmt +=  ",";
	stmt += temp.c_str();
	stmt +=  ");";	
	KLOG_WARNING(LOG_TAG, "le stmt batterie qui va arriver chez sqlite: %s \n", stmt.c_str());
	
/**
/data/data/essai.db
CREATE TABLE temp (ID INTEGER PRIMARY KEY AUTOINCREMENT, EPOCH INTEGER NOT NULL, BATT INTEGER NOT NULL, TEMP INTEGER NOT NULL);
sqlite3 /data/data/essai.db "select datetime(EPOCH, 'unixepoch','localtime'), (BATT*100/4041000), TEMP from temp;"
**/	
	
	rc = sqlite3_open("/data/data/essai.db", &db); 
	rc = sqlite3_exec(db, stmt.c_str(), NULL, 0, NULL);
	sqlite3_close(db);
	
}

void log_gps(float lat, float lng) {
	
	timespec ts;
	sqlite3 *db;
	int rc;	
	
	clock_gettime(CLOCK_REALTIME, &ts);	
	
	KLOG_WARNING(LOG_TAG, "lat: %f, lng: %f \n", lat, lng);

	std::string stmt;
	stmt = "insert into loc values(NULL,";
	stmt += std::to_string(ts.tv_sec);
	stmt +=  ",";
	stmt += std::to_string(lat);
	stmt +=  ",";
	stmt += std::to_string(lng);
	stmt +=  ");";	
	KLOG_WARNING(LOG_TAG, "le stmt loc qui va arriver chez sqlite: %s \n", stmt.c_str());

	//CREATE TABLE loc (ID INTEGER PRIMARY KEY AUTOINCREMENT, EPOCH INTEGER NOT NULL, LAT FLOAT NOT NULL, LONG FLOAT NOT NULL);
	//sqlite3 /data/data/loc.db "select datetime(EPOCH, 'unixepoch','localtime'), LAT, LONG from loc;"
	rc = sqlite3_open("/data/data/loc.db", &db); 
	rc = sqlite3_exec(db, stmt.c_str(), NULL, 0, NULL);
	sqlite3_close(db);	
	
	
	
	
}
