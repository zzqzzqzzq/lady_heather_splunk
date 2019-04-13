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

#include "parson.h"
#include "splunk.h"

// Hard coded configuration (Cope)
static char HEC_URL[] =  	"http://127.0.0.1:8088/services/collector";
static char HEC_TOKEN[] = 	"524ed854-7085-477f-93d9-ca00fe434f87";

// Yup, the error checking is pretty much non-existant.   What are we going to do? 
// Anything that's bad enough will kill Lady Heather, anything else will just get
// retried.

void splunk_log(void) { 
	JSON_Object 	*root_object;
	JSON_Object	*event_object;
	JSON_Object	*sat_object;

	JSON_Value	*critical_alarm_array;
	JSON_Value	*minor_alarm_array;
	// JSON_Value	*sat_array;

	char		rise[16];
	char		noon[16];
	char		set[16];

	int 		prn;
	int		num_sats;

	root_object = json_value_get_object( json_value_init_object() );
	event_object = splunk_json_initialize( root_object ); 	

// Alarms
	log_var_int(event_object, critical_alarms ); log_var_int(event_object, minor_alarms ); 

// Generate the upper left time info from show_time_info()
//
// alarm_time, alarm_date, egg_timer, cuckoo
// singing_clock, ships_clock

	log_var_int(event_object, alarm_date );
	log_var_int(event_object, alarm_time );

	log_var_long(event_object, egg_timer ); 
	log_var_int(event_object, cuckoo ); 
	log_var_int(event_object, singing_clock ); 
	log_var_int(event_object, ships_clock ); 

	log_var_int(event_object, time_flags );
	log_var_int(event_object, have_timing_mode ); 

	log_var_int(event_object, dst_ofs );
	log_var_int(event_object, leap_time );
	log_var_int(event_object, utc_offset ); 

	log_var_int(event_object,pri_hours); log_var_int(event_object,pri_minutes); log_var_int(event_object,pri_seconds); 
	log_var_funcstr(event_object, tz_info, tz_info() );

	log_var_int(event_object, have_week ); log_var_uint(event_object, gps_week ); 
	log_var_int(event_object, rolled ); log_var_double(event_object, rollover ); 
	log_var_int(event_object, have_tow ); log_var_uint(event_object, tow ); 

	log_var_int(event_object, have_utc_ofs ); log_var_int(event_object, utc_offset ); 

// show_operation_info(event_object,int why)
	log_var_int(event_object, rcvr_mode ); log_var_ulong(event_object, rcvr_type ); 
	log_var_str(event_object, unit_name );
	log_var_str(event_object, scpi_mfg ); 
	log_var_str(event_object, enviro_sn ); 

// show_lla
	// log_var_double(event_object, lat * RAD_TO_DEG); log_var_double(event_object, lon * RAD_TO_DEG); 
	splunk_json_add(event_object, "lat", json_value_init_number( (double) (lat * RAD_TO_DEG) ));
	splunk_json_add(event_object, "lon", json_value_init_number( (double) (lon * RAD_TO_DEG) ));
	log_var_double(event_object, alt); 

// show_survey_info
	log_var_int(event_object, precision_survey ); 
	log_var_int(event_object, survey_length ); 
	log_var_int(event_object, survey_secs ); 
	log_var_int(event_object, survey_progress ); 
	log_var_uint(event_object, precision_samples ); 

// @ 47679
	log_var_uint(event_object, discipline_mode );
	log_var_uint(event_object, holdover );
	log_var_uint(event_object, holdover_seen );

	log_var_int(event_object, scpi_life );

// 
	log_var_double(event_object, dac_voltage ); 
	log_var_int(event_object, pps_polarity ); 
	log_var_double(event_object, pps_offset );
	log_var_double(event_object, osc_offset );
	log_var_double(event_object, cable_delay );

// sun/moon data.

	log_var_double(event_object, sun_az );
	log_var_double(event_object, sun_el );
	log_var_double(event_object, moon_az );
	log_var_double(event_object, moon_el );
	//log_var_double(event_object, MoonPhase ); 
	splunk_json_add(event_object, "MoonPhase", json_value_init_number( (double) (MoonPhase * 100 )));
	log_var_double(event_object, MoonAge );

	log_var_int(event_object, do_moonrise );

	snprintf(rise, 16, "%02d:%02d:%02d", rise_hh, rise_mm, rise_ss);
	log_var_int(event_object, rise_hh ); 
	log_var_int(event_object, rise_mm ); 
	log_var_int(event_object, rise_ss ); 
	log_var_str(event_object, rise);

	snprintf(noon, 16, "%02d:%02d:%02d", noon_hh, noon_mm, noon_ss);
	log_var_int(event_object, noon_hh );
	log_var_int(event_object, noon_mm );
	log_var_int(event_object, noon_ss );
	log_var_str(event_object, noon); 

	snprintf(set, 16, "%02d:%02d:%02d", set_hh, set_mm, set_ss);
	log_var_int(event_object, set_hh );
	log_var_int(event_object, set_mm );
	log_var_int(event_object, set_ss );
	log_var_str(event_object, set);

	log_var_double(event_object, eot_ofs );
	log_var_double(event_object, ugals );

// 
	log_var_int(event_object, tfom ); 

	if     (ffom == 3) splunk_json_add(event_object, "ffom_txt", json_value_init_string("FFOM:INIT"));
	else if(ffom == 2) splunk_json_add(event_object, "ffom_txt", json_value_init_string("FFOM:UNLOCK"));
	else if(ffom == 1) splunk_json_add(event_object, "ffom_txt", json_value_init_string("FFOM:SETTLE"));
	else if(ffom == 0) splunk_json_add(event_object, "ffom_txt", json_value_init_string("FFOM:LOCK"));

	log_var_int(event_object, ffom); 

	log_var_float(event_object,  el_mask ); 

// show_critical_alarms()

	log_var_int(event_object, scpi_test);

	log_var_int(event_object, have_scpi_self_test);
	log_var_int(event_object, have_scpi_int_power);
	log_var_int(event_object, have_scpi_oven_power);
	log_var_int(event_object, have_scpi_ocxo);
	log_var_int(event_object, have_scpi_efc);
	log_var_int(event_object, have_scpi_gps);

	log_var_int(event_object, scpi_self_test);
	log_var_int(event_object, scpi_int_power);
	log_var_int(event_object, scpi_oven_power);
	log_var_int(event_object, scpi_ocxo);
	log_var_int(event_object, scpi_efc);
	log_var_int(event_object, scpi_gps);

	critical_alarm_array = json_value_init_array();
	if(critical_alarms & CRIT_PWR) json_array_append_value( json_array( critical_alarm_array ), json_value_init_string( "Power: Bad" )); 

	if(have_scpi_self_test && 	!scpi_self_test) 	json_array_append_value( json_array( critical_alarm_array ), json_value_init_string( "SCPI TEST: FAIL"));
	if(have_scpi_int_power && 	!scpi_int_power) 	json_array_append_value( json_array( critical_alarm_array ), json_value_init_string( "SCPI INTPWR: BAD"));
	if(have_scpi_oven_power && 	!scpi_oven_power) 	json_array_append_value( json_array( critical_alarm_array ), json_value_init_string( "SCPI OVEN: BAD"));
	if(have_scpi_ocxo && 		!scpi_ocxo) 		json_array_append_value( json_array( critical_alarm_array ), json_value_init_string( "SCPI OCXO: FAIL"));
	if(have_scpi_efc && 		!scpi_efc) 		json_array_append_value( json_array( critical_alarm_array ), json_value_init_string( "SCPI EFC: FAIL"));
	if(have_scpi_gps && 		!scpi_gps) 		json_array_append_value( json_array( critical_alarm_array ), json_value_init_string( "SCPI GPS: FAIL"));

	if(critical_alarms & CRIT_GPS) 	json_array_append_value( json_array( critical_alarm_array ), json_value_init_string( "GPS: BAD"));
	if(critical_alarms & CRIT_OCXO) json_array_append_value( json_array( critical_alarm_array ), json_value_init_string( "OSC: BAD"));
	if(critical_alarms & CRIT_ROM) 	json_array_append_value( json_array( critical_alarm_array ), json_value_init_string( "ROM/FLASH: BAD"));
	if(critical_alarms & CRIT_RAM) 	json_array_append_value( json_array( critical_alarm_array ), json_value_init_string( "RAM: BAD"));
	if(critical_alarms & CRIT_RTC) 	json_array_append_value( json_array( critical_alarm_array ), json_value_init_string( "RTC: BAD"));
	if(critical_alarms & CRIT_FPGA) json_array_append_value( json_array( critical_alarm_array ), json_value_init_string( "FPGA: BAD"));

	if( json_array_get_count( json_array(critical_alarm_array) ) == 0 ) {
		json_array_append_value( json_array( critical_alarm_array ), json_value_init_string( "No Critical Alarms"));
	}
	json_object_set_value( event_object, "critical_alarm_txt", critical_alarm_array );
	

// show_minor_alarms()  // Alarms should be logged fully. 

	minor_alarm_array = json_value_init_array();
	if(minor_alarms & MINOR_EEPROM) 		json_array_append_value( json_array( minor_alarm_array ), json_value_init_string("EEPROM BAD")); 

	if((minor_alarms & MINOR_ANT_OPEN) && (minor_alarms & MINOR_ANT_SHORT)) {
							json_array_append_value( json_array( minor_alarm_array ), json_value_init_string("Antenna no pwr"));
	} else if(minor_alarms & MINOR_ANT_OPEN) 	json_array_append_value( json_array( minor_alarm_array ), json_value_init_string("Antenna open"));
	  else if(minor_alarms & MINOR_ANT_SHORT) 	json_array_append_value( json_array( minor_alarm_array ), json_value_init_string("Antenna short"));

	if(minor_alarms & MINOR_ALMANAC )		json_array_append_value( json_array( minor_alarm_array ), json_value_init_string("No Almanac")); 

	if(minor_alarms & MINOR_OSC_CTRL ) {
		if(osc_control_on)		json_array_append_value( json_array( minor_alarm_array ), json_value_init_string("OSC PID CTRL"));
		else				json_array_append_value( json_array( minor_alarm_array ), json_value_init_string("Undisciplined"));
	}

	log_var_int( event_object, leap_days ); 
	if( minor_alarms & MINOR_LEAP_PEND ) {
		json_array_append_value( json_array( minor_alarm_array ), json_value_init_string("LeapPending"));
	} 

	if(minor_alarms & MINOR_TEST_MODE) 	json_array_append_value( json_array( minor_alarm_array ), json_value_init_string("Cal mode set")); 
	if(minor_alarms & MINOR_OSC_AGE) 	json_array_append_value( json_array( minor_alarm_array ), json_value_init_string("OSC Age Alarm")); 
	if(minor_alarms & MINOR_SURVEY) 	json_array_append_value( json_array( minor_alarm_array ), json_value_init_string("Self-Survey In Progress")); 
	if(minor_alarms & MINOR_NO_TRACK) 	json_array_append_value( json_array( minor_alarm_array ), json_value_init_string("No Satellites"));
	if(minor_alarms & MINOR_BAD_POSN) 	json_array_append_value( json_array( minor_alarm_array ), json_value_init_string("Saved Posn BAD")); 
	if(minor_alarms & MINOR_NO_POSN) 	json_array_append_value( json_array( minor_alarm_array ), json_value_init_string("No Saved Posn")); 
	if(minor_alarms & MINOR_PPS_SKIPPED) 	json_array_append_value( json_array( minor_alarm_array ), json_value_init_string("PPS: skipped"));
	if(minor_alarms & MINOR_UNKNOWN) 	json_array_append_value( json_array( minor_alarm_array ), json_value_init_string("Minor Alarm? Unknown")); 

	if( json_array_get_count( json_array(minor_alarm_array) ) == 0 ) {
		json_array_append_value( json_array( minor_alarm_array ), json_value_init_string( "No Minor Alarms"));
	}
	json_object_set_value( event_object, "minor_alarm_txt", minor_alarm_array );

	log_var_int( event_object, min_sig_level );
	log_var_int( event_object, max_sig_level );

	// Satellite information.
	num_sats = 0; 
	for(prn=0; prn<=MAX_PRN; prn++) {
		if( sat[prn].tracking != 0 ) { 
			sat_object = json_value_get_object( json_value_init_object() ); 
			splunk_log_sat( sat_object, prn ); 
	
    			// json_array_append_value(json_array( sat_array ), json_object_get_wrapping_value( sat_object )); 

			sprintf(prn_name, "sat_%04d", prn); 
			json_object_set_value( event_object, prn_name, json_object_get_wrapping_value( sat_object ) );
		}
		if( sat[prn].tracking > 0 ) { 
			num_sats++; 
		}
	}

	log_var_int( event_object, num_sats );

	sat_object = json_value_get_object( json_value_init_object() ); 
	splunk_log_sat( sat_object, SUN_PRN ); 

	json_object_set_value( event_object, "sat_SUN", json_object_get_wrapping_value( sat_object ));

    	// json_array_append_value(json_array( sat_array ), json_object_get_wrapping_value( sat_object )); 

	sat_object = json_value_get_object( json_value_init_object() ); 
	splunk_log_sat( sat_object, MOON_PRN ); 

	json_object_set_value( event_object, "sat_MOON", json_object_get_wrapping_value( sat_object ));
    	// json_array_append_value(json_array( sat_array ), json_object_get_wrapping_value( sat_object )); 


#ifdef DEBUG
	splunk_json_dump(root_object); 
#endif 

	splunk_json_send(HEC_TOKEN, HEC_URL, root_object);
	json_value_free( json_object_get_wrapping_value( root_object ) );
	return;
}

void splunk_log_sat( JSON_Object *sat_object, int prn ) { 

	splunk_json_add(sat_object, "prn",			json_value_init_number( (double) prn ));

// Interesting fields? 
	splunk_json_add(sat_object, "tracking",			json_value_init_number( (double) sat[prn].tracking ));
	splunk_json_add(sat_object, "elevation",		json_value_init_number( (double) sat[prn].elevation ));
	splunk_json_add(sat_object, "azimuth",			json_value_init_number( (double) sat[prn].azimuth ));
	splunk_json_add(sat_object, "sig_level",		json_value_init_number( (double) sat[prn].sig_level ));
	splunk_json_add(sat_object, "level_msg",		json_value_init_number( (double) sat[prn].level_msg ));
	splunk_json_add(sat_object, "disabled",			json_value_init_number( (double) sat[prn].disabled ));
	splunk_json_add(sat_object, "visible",			json_value_init_number( (double) sat[prn].visible ));
	splunk_json_add(sat_object, "el_dir",			json_value_init_number( (double) sat[prn].el_dir ));
	splunk_json_add(sat_object, "az_dir",			json_value_init_number( (double) sat[prn].az_dir ));

	splunk_json_add(sat_object, "health_flag", 		json_value_init_number( (double) sat[prn].health_flag )); 
	splunk_json_add(sat_object, "forced_healthy",		json_value_init_number( (double) sat[prn].forced_healthy ));

	return;

// These will never be logged, but it took too long to reformat them, so we won't just delete the code.
	splunk_json_add(sat_object, "sample_len",		json_value_init_number( (double) sat[prn].sample_len ));
	splunk_json_add(sat_object, "accum_range",		json_value_init_number( (double) sat[prn].accum_range ));
	splunk_json_add(sat_object, "raw_time",			json_value_init_number( (double) sat[prn].raw_time ));
	splunk_json_add(sat_object, "state",			json_value_init_number( (double) sat[prn].state ));
	splunk_json_add(sat_object, "code_phase",		json_value_init_number( (double) sat[prn].code_phase ));
	splunk_json_add(sat_object, "doppler",			json_value_init_number( (double) sat[prn].doppler ));
	splunk_json_add(sat_object, "range",			json_value_init_number( (double) sat[prn].range ));
	splunk_json_add(sat_object, "last_code_phase",		json_value_init_number( (double) sat[prn].last_code_phase ));
	splunk_json_add(sat_object, "last_cp",			json_value_init_number( (double) sat[prn].last_cp ));
	splunk_json_add(sat_object, "iii",			json_value_init_number( (double) sat[prn].iii ));
	splunk_json_add(sat_object, "last_doppler",		json_value_init_number( (double) sat[prn].last_doppler ));
	splunk_json_add(sat_object, "last_range",		json_value_init_number( (double) sat[prn].last_range ));
	splunk_json_add(sat_object, "ca_lli",			json_value_init_number( (double) sat[prn].ca_lli ));
	splunk_json_add(sat_object, "l1_level_msg",		json_value_init_number( (double) sat[prn].l1_level_msg ));
	splunk_json_add(sat_object, "l1_sig_level",		json_value_init_number( (double) sat[prn].l1_sig_level ));
	splunk_json_add(sat_object, "l1_code_phase",		json_value_init_number( (double) sat[prn].l1_code_phase ));
	splunk_json_add(sat_object, "l1_doppler",		json_value_init_number( (double) sat[prn].l1_doppler ));
	splunk_json_add(sat_object, "l1_range",			json_value_init_number( (double) sat[prn].l1_range ));
	splunk_json_add(sat_object, "last_l1_code_phase",	json_value_init_number( (double) sat[prn].last_l1_code_phase ));
	splunk_json_add(sat_object, "last_l1_doppler",		json_value_init_number( (double) sat[prn].last_l1_doppler ));
	splunk_json_add(sat_object, "last_l1_range",		json_value_init_number( (double) sat[prn].last_l1_range ));
	splunk_json_add(sat_object, "l1_lli",			json_value_init_number( (double) sat[prn].l1_lli ));
	splunk_json_add(sat_object, "l2_level_msg",		json_value_init_number( (double) sat[prn].l2_level_msg ));
	splunk_json_add(sat_object, "l2_sig_level",		json_value_init_number( (double) sat[prn].l2_sig_level ));
	splunk_json_add(sat_object, "l2_code_phase",		json_value_init_number( (double) sat[prn].l2_code_phase ));
	splunk_json_add(sat_object, "l2_doppler",		json_value_init_number( (double) sat[prn].l2_doppler ));
	splunk_json_add(sat_object, "l2_range",			json_value_init_number( (double) sat[prn].l2_range ));
	splunk_json_add(sat_object, "last_l2_code_phase",	json_value_init_number( (double) sat[prn].last_l2_code_phase ));
	splunk_json_add(sat_object, "last_l2_doppler",		json_value_init_number( (double) sat[prn].last_l2_doppler ));
	splunk_json_add(sat_object, "last_l2_range",		json_value_init_number( (double) sat[prn].last_l2_range ));
	splunk_json_add(sat_object, "l2_lli",			json_value_init_number( (double) sat[prn].l2_lli ));
	splunk_json_add(sat_object, "l5_level_msg",		json_value_init_number( (double) sat[prn].l5_level_msg ));
	splunk_json_add(sat_object, "l5_sig_level",		json_value_init_number( (double) sat[prn].l5_sig_level ));
	splunk_json_add(sat_object, "l5_code_phase",		json_value_init_number( (double) sat[prn].l5_code_phase ));
	splunk_json_add(sat_object, "l5_doppler",		json_value_init_number( (double) sat[prn].l5_doppler ));
	splunk_json_add(sat_object, "l5_range",			json_value_init_number( (double) sat[prn].l5_range ));
	splunk_json_add(sat_object, "last_l5_code_phase",	json_value_init_number( (double) sat[prn].last_l5_code_phase ));
	splunk_json_add(sat_object, "last_l5_doppler",		json_value_init_number( (double) sat[prn].last_l5_doppler ));
	splunk_json_add(sat_object, "last_l5_range",		json_value_init_number( (double) sat[prn].last_l5_range ));
	splunk_json_add(sat_object, "l5_lli",			json_value_init_number( (double) sat[prn].l5_lli ));
	splunk_json_add(sat_object, "l6_level_msg",		json_value_init_number( (double) sat[prn].l6_level_msg ));
	splunk_json_add(sat_object, "l6_sig_level",		json_value_init_number( (double) sat[prn].l6_sig_level ));
	splunk_json_add(sat_object, "l6_code_phase",		json_value_init_number( (double) sat[prn].l6_code_phase ));
	splunk_json_add(sat_object, "l6_doppler",		json_value_init_number( (double) sat[prn].l6_doppler ));
	splunk_json_add(sat_object, "l6_range",			json_value_init_number( (double) sat[prn].l6_range ));
	splunk_json_add(sat_object, "last_l6_code_phase",	json_value_init_number( (double) sat[prn].last_l6_code_phase ));
	splunk_json_add(sat_object, "last_l6_doppler",		json_value_init_number( (double) sat[prn].last_l6_doppler ));
	splunk_json_add(sat_object, "last_l6_range",		json_value_init_number( (double) sat[prn].last_l6_range ));
	splunk_json_add(sat_object, "l6_lli",			json_value_init_number( (double) sat[prn].l6_lli ));
	splunk_json_add(sat_object, "l7_level_msg",		json_value_init_number( (double) sat[prn].l7_level_msg ));
	splunk_json_add(sat_object, "l7_sig_level",		json_value_init_number( (double) sat[prn].l7_sig_level ));
	splunk_json_add(sat_object, "l7_code_phase",		json_value_init_number( (double) sat[prn].l7_code_phase ));
	splunk_json_add(sat_object, "l7_doppler",		json_value_init_number( (double) sat[prn].l7_doppler ));
	splunk_json_add(sat_object, "l7_range",			json_value_init_number( (double) sat[prn].l7_range ));
	splunk_json_add(sat_object, "last_l7_code_phase",	json_value_init_number( (double) sat[prn].last_l7_code_phase ));
	splunk_json_add(sat_object, "last_l7_doppler",		json_value_init_number( (double) sat[prn].last_l7_doppler ));
	splunk_json_add(sat_object, "last_l7_range",		json_value_init_number( (double) sat[prn].last_l7_range ));
	splunk_json_add(sat_object, "l7_lli",			json_value_init_number( (double) sat[prn].l7_lli ));
	splunk_json_add(sat_object, "l8_level_msg",		json_value_init_number( (double) sat[prn].l8_level_msg ));
	splunk_json_add(sat_object, "l8_sig_level",		json_value_init_number( (double) sat[prn].l8_sig_level ));
	splunk_json_add(sat_object, "l8_code_phase",		json_value_init_number( (double) sat[prn].l8_code_phase ));
	splunk_json_add(sat_object, "l8_doppler",		json_value_init_number( (double) sat[prn].l8_doppler ));
	splunk_json_add(sat_object, "l8_range",			json_value_init_number( (double) sat[prn].l8_range ));
	splunk_json_add(sat_object, "last_l8_code_phase",	json_value_init_number( (double) sat[prn].last_l8_code_phase ));
	splunk_json_add(sat_object, "last_l8_doppler",		json_value_init_number( (double) sat[prn].last_l8_doppler ));
	splunk_json_add(sat_object, "last_l8_range",		json_value_init_number( (double) sat[prn].last_l8_range ));
	splunk_json_add(sat_object, "l8_lli",			json_value_init_number( (double) sat[prn].l8_lli ));
	splunk_json_add(sat_object, "eph_time",			json_value_init_number( (double) sat[prn].eph_time ));
	splunk_json_add(sat_object, "eph_health",		json_value_init_number( (double) sat[prn].eph_health ));
	splunk_json_add(sat_object, "iode",			json_value_init_number( (double) sat[prn].iode ));
	splunk_json_add(sat_object, "toe",			json_value_init_number( (double) sat[prn].toe ));
	splunk_json_add(sat_object, "fit_flag",			json_value_init_number( (double) sat[prn].fit_flag ));
	splunk_json_add(sat_object, "sv_accuracy",		json_value_init_number( (double) sat[prn].sv_accuracy ));
	splunk_json_add(sat_object, "chan",			json_value_init_number( (double) sat[prn].chan ));
	splunk_json_add(sat_object, "acq_flag",			json_value_init_number( (double) sat[prn].acq_flag ));
	splunk_json_add(sat_object, "eph_flag",			json_value_init_number( (double) sat[prn].eph_flag ));
	splunk_json_add(sat_object, "time_of_week",		json_value_init_number( (double) sat[prn].time_of_week ));
	splunk_json_add(sat_object, "age",			json_value_init_number( (double) sat[prn].age ));
	splunk_json_add(sat_object, "msec",			json_value_init_number( (double) sat[prn].msec ));
	splunk_json_add(sat_object, "bad_flag",			json_value_init_number( (double) sat[prn].bad_flag ));
	splunk_json_add(sat_object, "collecting",		json_value_init_number( (double) sat[prn].collecting ));
	splunk_json_add(sat_object, "how_used",			json_value_init_number( (double) sat[prn].how_used ));
	splunk_json_add(sat_object, "sv_type",			json_value_init_number( (double) sat[prn].sv_type ));
	splunk_json_add(sat_object, "osa_snr",			json_value_init_number( (double) sat[prn].osa_snr ));
	splunk_json_add(sat_object, "osa_state",		json_value_init_number( (double) sat[prn].osa_state ));
	splunk_json_add(sat_object, "plot_x",			json_value_init_number( (double) sat[prn].plot_x ));
	splunk_json_add(sat_object, "plot_y",			json_value_init_number( (double) sat[prn].plot_y ));
	splunk_json_add(sat_object, "plot_r",			json_value_init_number( (double) sat[prn].plot_r ));
	splunk_json_add(sat_object, "plot_color",		json_value_init_number( (double) sat[prn].plot_color ));
	splunk_json_add(sat_object, "sat_bias",			json_value_init_number( (double) sat[prn].sat_bias ));
	splunk_json_add(sat_object, "time_of_fix",		json_value_init_number( (double) sat[prn].time_of_fix ));
	splunk_json_add(sat_object, "last_bias_msg",		json_value_init_number( (double) sat[prn].last_bias_msg ));
	splunk_json_add(sat_object, "eph_valid",		json_value_init_number( (double) sat[prn].eph_valid ));
	splunk_json_add(sat_object, "t_ephem",			json_value_init_number( (double) sat[prn].t_ephem ));
	splunk_json_add(sat_object, "eph_week",			json_value_init_number( (double) sat[prn].eph_week ));
	splunk_json_add(sat_object, "codeL2",			json_value_init_number( (double) sat[prn].codeL2 ));
	splunk_json_add(sat_object, "L2Pdata",			json_value_init_number( (double) sat[prn].L2Pdata ));
	splunk_json_add(sat_object, "sv_accu_raw",		json_value_init_number( (double) sat[prn].sv_accu_raw ));
	splunk_json_add(sat_object, "sv_health",		json_value_init_number( (double) sat[prn].sv_health ));
	splunk_json_add(sat_object, "iodc",			json_value_init_number( (double) sat[prn].iodc ));
	splunk_json_add(sat_object, "tGD",			json_value_init_number( (double) sat[prn].tGD ));
	splunk_json_add(sat_object, "toc",			json_value_init_number( (double) sat[prn].toc ));
	splunk_json_add(sat_object, "af2",			json_value_init_number( (double) sat[prn].af2 ));
	splunk_json_add(sat_object, "af1",			json_value_init_number( (double) sat[prn].af1 ));
	splunk_json_add(sat_object, "af0",			json_value_init_number( (double) sat[prn].af0 ));
	splunk_json_add(sat_object, "eph_sv_accu",		json_value_init_number( (double) sat[prn].eph_sv_accu ));
	splunk_json_add(sat_object, "eph_iode",			json_value_init_number( (double) sat[prn].eph_iode ));
	splunk_json_add(sat_object, "fit_interval",		json_value_init_number( (double) sat[prn].fit_interval ));
	splunk_json_add(sat_object, "Crs",			json_value_init_number( (double) sat[prn].Crs ));
	splunk_json_add(sat_object, "delta_n",			json_value_init_number( (double) sat[prn].delta_n ));
	splunk_json_add(sat_object, "M0",			json_value_init_number( (double) sat[prn].M0 ));
	splunk_json_add(sat_object, "Cuc",			json_value_init_number( (double) sat[prn].Cuc ));
	splunk_json_add(sat_object, "e",			json_value_init_number( (double) sat[prn].e ));
	splunk_json_add(sat_object, "Cus",			json_value_init_number( (double) sat[prn].Cus ));
	splunk_json_add(sat_object, "sqrtA",			json_value_init_number( (double) sat[prn].sqrtA ));
	splunk_json_add(sat_object, "eph_toe",			json_value_init_number( (double) sat[prn].eph_toe ));
	splunk_json_add(sat_object, "Cic",			json_value_init_number( (double) sat[prn].Cic ));
	splunk_json_add(sat_object, "omega_0",			json_value_init_number( (double) sat[prn].omega_0 ));
	splunk_json_add(sat_object, "Cis",			json_value_init_number( (double) sat[prn].Cis ));
	splunk_json_add(sat_object, "io",			json_value_init_number( (double) sat[prn].io ));
	splunk_json_add(sat_object, "Crc",			json_value_init_number( (double) sat[prn].Crc ));
	splunk_json_add(sat_object, "omega",			json_value_init_number( (double) sat[prn].omega ));
	splunk_json_add(sat_object, "omega_dot",		json_value_init_number( (double) sat[prn].omega_dot ));
	splunk_json_add(sat_object, "i_dot",			json_value_init_number( (double) sat[prn].i_dot ));
	splunk_json_add(sat_object, "axis",			json_value_init_number( (double) sat[prn].axis ));
	splunk_json_add(sat_object, "n",			json_value_init_number( (double) sat[prn].n ));
	splunk_json_add(sat_object, "r1me2",			json_value_init_number( (double) sat[prn].r1me2 ));
	splunk_json_add(sat_object, "omega_n",			json_value_init_number( (double) sat[prn].omega_n ));
	splunk_json_add(sat_object, "odot_n",			json_value_init_number( (double) sat[prn].odot_n ));

	return;
}
