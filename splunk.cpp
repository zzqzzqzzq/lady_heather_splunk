/*
 * Log Lady Heather into Splunk
 *
 */

#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <errno.h>
#include <time.h>
#include <curl/curl.h>

#define EXTERN extern		// So we can include heather.ch
#include "heather.ch"

#include "splunk.h"

void splunk_log(void) { 
	char 	element[MAX_ELEMENT]; 
	char	*array_buf = NULL;

	splunk_json_initialize(); 

// Alarms
	log_var_int( critical_alarms ); log_var_int( minor_alarms ); 

// Generate the upper left time info from show_time_info()
//
// alarm_time, alarm_date, egg_timer, cuckoo
// singing_clock, ships_clock

	log_var_int( alarm_date );
	log_var_int( alarm_time );

	log_var_long( egg_timer ); 
	log_var_int( cuckoo ); 
	log_var_int( singing_clock ); 
	log_var_int( ships_clock ); 

	log_var_int( time_flags );
	log_var_int( have_timing_mode ); 

	log_var_int( dst_ofs );
	log_var_int( leap_time );
	log_var_int( utc_offset ); 

	log_var_int(pri_hours); log_var_int(pri_minutes); log_var_int(pri_seconds); 
	log_var_funcstr( tz_info, tz_info() );

	log_var_int( have_week ); log_var_uint( gps_week ); 
	log_var_int( rolled ); log_var_double( rollover ); 
	log_var_int( have_tow ); log_var_uint( tow ); 

	log_var_int( have_utc_ofs ); log_var_int( utc_offset ); 

// show_operation_info(int why)
	log_var_int( rcvr_mode ); log_var_ulong( rcvr_type ); 
	log_var_str( unit_name );
	log_var_str( scpi_mfg ); 
	log_var_str( enviro_sn ); 

// show_lla
	log_var_double(lat); log_var_double(lon); log_var_double(alt); 

// show_survey_info
	log_var_int( precision_survey ); 
	log_var_int( survey_length ); 
	log_var_int( survey_secs ); 
	log_var_int( survey_progress ); 
	log_var_uint( precision_samples ); 

// @ 47679
	log_var_uint( discipline_mode );
	log_var_uint( holdover );
	log_var_uint( holdover_seen );

	log_var_int( scpi_life );

// 
	log_var_double( dac_voltage ); 
	log_var_int( pps_polarity ); 
	log_var_double( pps_offset );
	log_var_double( osc_offset );
	log_var_double( cable_delay );

// sun/moon data.

	log_var_double( sun_az );
	log_var_double( sun_el );
	log_var_double( moon_az );
	log_var_double( moon_el );
	log_var_double( MoonPhase ); 
	log_var_double( MoonAge );

	log_var_int( do_moonrise );
	log_var_int( rise_hh ); 
	log_var_int( rise_mm ); 
	log_var_int( rise_ss ); 

	log_var_int( noon_hh );
	log_var_int( noon_mm );
	log_var_int( noon_ss );

	log_var_int( set_hh );
	log_var_int( set_mm );
	log_var_int( set_ss );

	log_var_double( eot_ofs );
	log_var_double( ugals );

// 
	log_var_int( tfom ); 

	if     (ffom == 3) splunk_json_add("ffom_txt", "FFOM:INIT");
	else if(ffom == 2) splunk_json_add("ffom_txt", "FFOM:UNLOCK");
	else if(ffom == 1) splunk_json_add("ffom_txt", "FFOM:SETTLE");
	else if(ffom == 0) splunk_json_add("ffom_txt", "FFOM:LOCK");

	log_var_int(ffom); 

	log_var_float( el_mask ); 

// show_critical_alarms()

	log_var_int(scpi_test);

	log_var_int(scpi_self_test);
	log_var_int(scpi_int_power);
	log_var_int(scpi_oven_power);
	log_var_int(scpi_ocxo);
	log_var_int(scpi_efc);
	log_var_int(scpi_gps);

	array_buf = NULL;
	if(critical_alarms & CRIT_PWR) array_buf = splunk_array_add_string(array_buf, "Power: BAD");

	if(!scpi_self_test) array_buf = splunk_array_add_string(array_buf, "SCPI TEST: FAIL");
	if(!scpi_int_power) array_buf = splunk_array_add_string(array_buf, "SCPI INTPWR: BAD");
	if(!scpi_oven_power) array_buf = splunk_array_add_string(array_buf, "SCPI OVEN: BAD");
	if(!scpi_ocxo) array_buf = splunk_array_add_string(array_buf, "SCPI OCXO: FAIL");
	if(!scpi_efc) array_buf = splunk_array_add_string(array_buf, "SCPI EFC: FAIL");
	if(!scpi_gps) array_buf = splunk_array_add_string(array_buf, "SCPI GPS: FAIL");

	if(critical_alarms & CRIT_GPS) array_buf = splunk_array_add_string(array_buf, "GPS: BAD");
	if(critical_alarms & CRIT_OCXO) array_buf = splunk_array_add_string(array_buf, "OSC: BAD");
	if(critical_alarms & CRIT_ROM) array_buf = splunk_array_add_string(array_buf, "ROM/FLASH: BAD");
	if(critical_alarms & CRIT_RAM) array_buf = splunk_array_add_string(array_buf, "RAM: BAD");
	if(critical_alarms & CRIT_RTC) array_buf = splunk_array_add_string(array_buf, "RTC: BAD");
	if(critical_alarms & CRIT_FPGA) array_buf = splunk_array_add_string(array_buf, "FPGA: BAD");
 
	splunk_array_end( array_buf, "critical_alarms_txt", "No Critical Alarms" );

// show_minor_alarms()  // Alarms should be logged fully. 

	array_buf = NULL;
	if(minor_alarms & MINOR_EEPROM) array_buf = splunk_array_add_string(array_buf, "EEPROM BAD"); 

	if((minor_alarms & MINOR_ANT_OPEN) && (minor_alarms & MINOR_ANT_SHORT)) {
							array_buf = splunk_array_add_string(array_buf, "Antenna no pwr");
	} else if(minor_alarms & MINOR_ANT_OPEN) 	array_buf = splunk_array_add_string(array_buf, "Antenna open");
	  else if(minor_alarms & MINOR_ANT_SHORT) 	array_buf = splunk_array_add_string(array_buf, "Antenna short");

	if(minor_alarms & MINOR_ALMANAC )	array_buf = splunk_array_add_string(array_buf, "No Almanac"); 

	if(minor_alarms & MINOR_OSC_CTRL ) {
		if(osc_control_on)		array_buf = splunk_array_add_string(array_buf, "OSC PID CTRL");
		else				array_buf = splunk_array_add_string(array_buf, "Undisciplined");
	}

	log_var_int( leap_days ); 
	if( minor_alarms & MINOR_LEAP_PEND ) {
		array_buf = splunk_array_add_string(array_buf, "LeapPending");
	} 

	if(minor_alarms & MINOR_TEST_MODE) 	array_buf = splunk_array_add_string(array_buf, "Cal mode set"); 
	if(minor_alarms & MINOR_OSC_AGE) 	array_buf = splunk_array_add_string(array_buf, "OSC Age Alarm"); 
	if(minor_alarms & MINOR_SURVEY) 	array_buf = splunk_array_add_string(array_buf, "Self-Survey In Progress"); 
	if(minor_alarms & MINOR_NO_TRACK) 	array_buf = splunk_array_add_string(array_buf, "No Satellites");
	if(minor_alarms & MINOR_BAD_POSN) 	array_buf = splunk_array_add_string(array_buf, "Saved Posn BAD"); 
	if(minor_alarms & MINOR_NO_POSN) 	array_buf = splunk_array_add_string(array_buf, "No Saved Posn"); 
	if(minor_alarms & MINOR_PPS_SKIPPED) 	array_buf = splunk_array_add_string(array_buf, "PPS: skipped");
	if(minor_alarms & MINOR_UNKNOWN) 	array_buf = splunk_array_add_string(array_buf, "Minor Alarm? Unknown"); 

	splunk_array_end( array_buf, "minor_alarm_txt", "No Minor Alarms" );

	return;
}

