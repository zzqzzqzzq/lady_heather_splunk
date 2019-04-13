/*
 * Log Lady Heather into Splunk
 *
 * Sample JSON:
 *
 * { "time": "1552935031.81857",		// wall_time
 *   "dac_voltage": "-10.1965",
 *   "discipline_mode": "0",
 *   "day", "18"
 * }
 * 
 */

#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <errno.h>
#include <time.h>

#define EXTERN extern		// So we can include heather.ch
#include "heather.ch"

#include "parson.h"
#include "splunk.h"

#define MAX_JSON		1048576		// Maximum JSON data POST size per default Splunk (1Mb)

void splunk_json_dump(JSON_Object *buf) { 
	FILE	*fptr; 
	char	*serialized_string;

	fptr = fopen("splunk_debug.txt", "w"); 

	serialized_string = json_serialize_to_string_pretty( json_object_get_wrapping_value( buf ) );
	fprintf(fptr, "%s", serialized_string );

	fclose(fptr);
	json_free_serialized_string( serialized_string ); 

	return;
}

JSON_Object *splunk_json_initialize(JSON_Object *buf) { 
	JSON_Object	*event_object;

	splunk_json_add(buf, "time", 		json_value_init_number( (double) wall_time ));
	splunk_json_add(buf, "sourcetype", 	json_value_init_string( "lady_heather:" VERSION ));

	event_object = json_value_get_object( json_value_init_object() );
	json_object_set_value( buf, "event", json_object_get_wrapping_value( event_object ) );

#ifdef DEBUG2
	splunk_json_dump(buf); 
#endif 
	return( event_object ); 
}

int splunk_json_add(JSON_Object *buf, char *name, JSON_Value *value) { 
	json_object_set_value( buf, name, value );

#ifdef DEBUG2
	splunk_json_dump( buf ); 
#endif

	// json_value_free( value );				// Do not free value, till the entire tree is done.
	return(0);
}

#ifdef TEST
double wall_time = (double)time(0); 

int main(int argc, char **argv) { 
	float	pi = 3.1415;
	char	woot[] = "Wootabega!";

	JSON_Object 	*root_object;
	JSON_Object	*event_object;

	JSON_Value	*array_value;

	JSON_Object	*sat_object; 

	char 		*serialized_string;
	int		i; 

	root_object = json_value_get_object( json_value_init_object() );
	event_object = splunk_json_initialize( root_object ); 

	log_var_double( event_object, wall_time );
	log_var_double( event_object, pi );
	log_var_str( event_object, woot ); 

	array_value = json_value_init_array();
	for( i = 0; i < 3; i ++ ) { 
		sat_object = json_value_get_object( json_value_init_object() ); 

		// log_var_int( sat_object, "prn", i );
		// log_var_float( sat_object, "elevation", i * 3.1415 );
		// log_var_int( sat_object, "tracking", i & 1 ); 
		// log_var_int( sat_object, "visible", 1 ); 

		splunk_json_add(sat_object, "prn", 		json_value_init_number( (double) i )); 
		splunk_json_add(sat_object, "elevation",	json_value_init_number( (double) i * 3.1415 )); 
		splunk_json_add(sat_object, "tracking",		json_value_init_number( (double) (i & 1) )); 
		splunk_json_add(sat_object, "visible",		json_value_init_number( (double) 1 )); 

    		json_array_append_value(json_array( array_value ), json_object_get_wrapping_value( sat_object )); 
	}

	json_object_set_value( event_object, "sat", array_value );

	// Display output.
	serialized_string = json_serialize_to_string_pretty( json_object_get_wrapping_value( root_object ) ); 
	printf("%s\n", serialized_string); 

	// Cleanup.
	json_free_serialized_string( serialized_string );
	json_value_free( json_object_get_wrapping_value( root_object ) );

	return(1);
}
#endif
