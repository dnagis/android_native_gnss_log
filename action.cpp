#include "header.h"
#include <cutils/klog.h>
#include <android-base/file.h> //libbase @ LOCAL_SHARED_LIBRARIES
#include <sqlite3.h> //libsqlite @ LOCAL_SHARED_LIBRARIES 
#include <fstream>

#define LOG_TAG "gpsvvnx"
#define KLOG_LEVEL 6




#include <hidl/Status.h>
#include <android/hardware/gnss/1.0/IGnss.h>
#include <binder/IServiceManager.h>

#include "stdio_filebuf.h" //voir native playground interface binder dumpsys

using android::hardware::Return;
using android::hardware::Void;

using android::hardware::gnss::V1_0::IGnss;
using android::hardware::gnss::V1_0::IGnssCallback;
using android::hardware::gnss::V1_0::GnssLocation;
using android::sp;

using android::String16;
using namespace android;

//Si la totalité des mthdes ne sont pas implémentées erreur peu informative:
//error: allocating an object of abstract class type 'GnssCallback'
class GnssCallback : public IGnssCallback {
	public:	
	
	virtual ~GnssCallback() = default;
	
    Return<void> gnssLocationCb(const GnssLocation& location) override {
      //fprintf(stdout, "Location received... %f %f %f %f\n", location.latitudeDegrees, location.longitudeDegrees, location.horizontalAccuracyMeters, location.verticalAccuracyMeters); //gnss/1.0/types.hal
      log_gps(location.latitudeDegrees, location.longitudeDegrees);
      return Void();
    }

    Return<void> gnssStatusCb(
        const IGnssCallback::GnssStatusValue /* status */) override {
      return Void();
    }
    
    Return<void> gnssSvStatusCb(
        const IGnssCallback::GnssSvStatus& /* svStatus */) override {
      return Void();
    }
    
    Return<void> gnssNmeaCb(
        int64_t /* timestamp */,
        const android::hardware::hidl_string& /* nmea */) override {
      return Void();
    }
    
    Return<void> gnssSetCapabilitesCb(uint32_t capabilities) override {
      fprintf(stdout,"Capabilities received %d\n", capabilities);
      return Void();
    }
    
    Return<void> gnssAcquireWakelockCb() override { return Void(); }
    Return<void> gnssReleaseWakelockCb() override { return Void(); }
    Return<void> gnssRequestTimeCb() override { return Void(); }
    
    Return<void> gnssSetSystemInfoCb(
        const IGnssCallback::GnssSystemInfo& info) override {
      fprintf(stdout,"Info received, year %d\n", info.yearOfHw);
      return Void();
    }


};

void action() {

	timespec ts;
	sqlite3 *db;
	int rc;		
	std::string stmt, charge, temp, time_string, line, idle_status;
	Vector<String16> args;
	int pipefd[2];
	
	
	clock_gettime(CLOCK_REALTIME, &ts);
	time_string = ctime(&ts.tv_sec); //https://linux.die.net/man/3/ctime	    
	
	std::ifstream ifile_c("/sys/class/power_supply/battery/charge_counter");
    ifile_c >> charge; 
    ifile_c.close();
    
    std::ifstream ifile_t("/sys/class/power_supply/battery/temp");
    ifile_t >> temp; 
    ifile_t.close();
    

	/**
	 * dumpsys 	//deviceidle--> voir native playground interface binder dumpsys pour explications
	 ***/
	  


	/**récupération d'une variable dans un dumpsys**/
	if (pipe(pipefd) == -1) {
        KLOG_WARNING(LOG_TAG, "error pipe \n");
    }
	 
	android::sp<android::IBinder> binder_idle = android::defaultServiceManager()->checkService(android::String16("deviceidle"));
    if (binder_idle == NULL) {
		 KLOG_WARNING(LOG_TAG, "gpsvvnx dans action: check service idle a plante\n");
	} else { binder_idle->dump(pipefd[1], args);}
	
	FILE* pFile = fdopen(pipefd[0], "r");
    if (pFile == NULL) printf("Error opening pipe");
	stdio_filebuf<char> filebuf(pFile);
    std::istream is(&filebuf);
    
    while(std::getline(is, line)) 
    {
		//printf("%lu -- %s\n", line.length(), line.c_str());
		if (line.find("mState") != std::string::npos) 
		{
		std::size_t start = 2;
		size_t ws = line.find(" ", start);
		//printf("ws @ %lu", ws);
		idle_status = line.substr(9, ws-9);
		break;
		}
	}
	//idle_status = "dummy";
	//close(pipefd[0]);
	//close(pipefd[1]); 
	
	
	
	
	/**idle, écriture dans un fichier
	FILE* fp = fopen("/data/data/idle.txt", "a");
	fd = fileno(fp);
	dprintf(fd, "********@ %s\n", time_string.c_str());		 
	android::sp<android::IBinder> binder_pwr = android::defaultServiceManager()->checkService(android::String16("deviceidle"));
    if (binder_pwr == NULL) {
		 KLOG_WARNING(LOG_TAG, "gpsvvnx dans action: check service power a plante\n");
	} else { binder_pwr->dump(fd, args);}
	fclose(fp); **/

	
	/**power, écriture dans un fichier
	fp = fopen("/data/data/power.txt", "a");
	fd = fileno(fp);
	dprintf(fd, "********@ %s\n", time_string.c_str());		 
	android::sp<android::IBinder> binder_pwr = android::defaultServiceManager()->checkService(android::String16("power"));
    if (binder_pwr == NULL) {
		 KLOG_WARNING(LOG_TAG, "gpsvvnx dans action: check service power a plante\n");
	} else { binder_pwr->dump(fd, args);}
	fclose(fp); **/
	
	
	KLOG_WARNING(LOG_TAG, "********Timer Triggered******* @ %ld charge = %s temp = %s idle status = %s\n", (long)ts.tv_sec, charge.c_str(), temp.c_str(), idle_status.c_str());
	
	//build string stmt à passer à sqlite
	stmt = "insert into power values(NULL,";
	stmt += std::to_string(ts.tv_sec);
	stmt +=  ",";
	stmt += charge.c_str();
	stmt +=  ",";
	stmt += temp.c_str();
	stmt +=  ",'";
	stmt += idle_status.c_str(); //attention c du text faut entourer par ' '
	stmt +=  "');";	
	KLOG_WARNING(LOG_TAG, "le stmt batterie qui va arriver chez sqlite: %s \n", stmt.c_str());
	
	/**
	/data/data/powerlog.db
	CREATE TABLE power (ID INTEGER PRIMARY KEY AUTOINCREMENT, EPOCH INTEGER NOT NULL, BATT INTEGER NOT NULL, TEMP INTEGER NOT NULL, IDLE TEXT NOT NULL);
	sqlite3 /data/data/powerlog.db "select datetime(EPOCH, 'unixepoch','localtime'), (BATT*100/4041000), TEMP, IDLE from power;"
	**/	
	
	rc = sqlite3_open("/data/data/powerlog.db", &db); 
	rc = sqlite3_exec(db, stmt.c_str(), NULL, 0, NULL);
	sqlite3_close(db);
	
	
	
	
	
	/**
	HAL GNSS
	**/	
	KLOG_WARNING(LOG_TAG, "on arrive à la partie hal  \n");
	bool result;
	
	
	if (idle_status != "ZOOOOOBBBBB") return; //pour ne pas aller plus loin pour tests batterie
	
	sp<IGnss> gnss_hal = IGnss::getService();
	if (gnss_hal == nullptr) KLOG_WARNING(LOG_TAG, "null_ptr hal...\n");
	KLOG_WARNING(LOG_TAG, "on a le service \n");
	
	sp<IGnssCallback> gnss_cb = new GnssCallback();
    if (gnss_cb == nullptr) KLOG_WARNING(LOG_TAG, "null_ptr cb...\n");
    
    result = gnss_hal->setCallback(gnss_cb);
	if (!result) fprintf(stderr, "erreur setcb...\n");
	
	result = gnss_hal->setPositionMode(
      IGnss::GnssPositionMode::MS_BASED,
      IGnss::GnssPositionRecurrence::RECURRENCE_PERIODIC, 10000,
      0, 0);

	result = gnss_hal->stop();
	if (!result) KLOG_WARNING(LOG_TAG, "erreur stop...\n");
	
	if(idle_status == "IDLE") return;

	sleep(5);
	
    result = gnss_hal->start();
    if (!result) KLOG_WARNING(LOG_TAG, "erreur start...\n");
	
	
	
}

void log_gps(float lat, float lng) {
	
	timespec ts;
	sqlite3 *db;
	int rc;
	FILE * pFile;	
	
	clock_gettime(CLOCK_REALTIME, &ts);	
	
	KLOG_WARNING(LOG_TAG, "lat: %0.14f, lng: %0.14f \n", lat, lng);
	
	char lat_precision[20], lng_precision[20];
	sprintf(lat_precision, "%0.14f", lat);
	sprintf(lng_precision, "%0.14f", lng);
	

	std::string stmt;
	stmt = "insert into loc values(NULL,";
	stmt += std::to_string(ts.tv_sec);
	stmt +=  ",";
	stmt += lat_precision;
	stmt +=  ",";
	stmt += lng_precision;
	stmt +=  ");";	
	KLOG_WARNING(LOG_TAG, "le stmt loc qui va arriver chez sqlite: %s \n", stmt.c_str());

	//CREATE TABLE loc (ID INTEGER PRIMARY KEY AUTOINCREMENT, EPOCH INTEGER NOT NULL, LAT FLOAT NOT NULL, LONG FLOAT NOT NULL);
	//sqlite3 /data/data/loc.db "select datetime(EPOCH, 'unixepoch','localtime'), LAT, LONG from loc;"
	rc = sqlite3_open("/data/data/loc.db", &db); 
	rc = sqlite3_exec(db, stmt.c_str(), NULL, 0, NULL);
	sqlite3_close(db);	
	
	
	pFile = fopen("/sdcard/gps.txt", "w+");
	fprintf(pFile, "%s %0.14f %0.14f", std::to_string(ts.tv_sec).c_str(), lat, lng);		 
	fclose(pFile);
	
	
	
	
}
