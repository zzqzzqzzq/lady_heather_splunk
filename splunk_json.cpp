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
#include <curl/curl.h>

#define EXTERN extern		// So we can include heather.ch
#include "heather.ch"

#include "splunk.h"

// Used for retrieving responses from via libCURL
struct MemoryStruct {
	char *memory;
	size_t size;
};

// Hard coded configuration for initial testing
#define HEC_URL 	"http://peace.internaldomain.us:8088/services/collector"
#define HEC_TOKEN	"524ed854-7085-477f-93d9-ca00fe434f87"

#define MAX_JSON	1048576

static char 	splunk_json_buffer[ MAX_JSON ]; 			// Maximum size of post to Splunk (1MB)
static int	splunk_json_event_count = 0; 				// Counter to number of events, we cannot send an "empty" (0) list.

#ifdef TEST
double wall_time = (double)time(0); 
#endif

#ifdef DEBUG
void splunk_json_dump(char *buf) { 
	FILE	*fptr; 

	fptr = fopen("splunk_debug.txt", "w"); 
	fprintf(fptr, "%s", buf);
	fclose(fptr);
	return;
}
#endif

int splunk_json_initialize(void) { 
	memset( &splunk_json_buffer, 0, sizeof( splunk_json_buffer )); 
	snprintf(splunk_json_buffer, MAX_JSON, "{ \"time\": \"%lf\", \"sourcetype\": \"lady_heather:%s\",  \"event\": {\n  }\n}", wall_time, VERSION ); 
	splunk_json_event_count = 0; 

#ifdef DEBUG
	splunk_json_dump(splunk_json_buffer); 
#endif 
	return(0); 
}

// char *splunk_hasharray_add_int( char *buf, int value ) { 

//////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * Simple JSON ARRAYS
 */
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void splunk_array_end( char *buf, char *name, char *empty_default ) {

	if( buf == NULL ) { 
		buf = splunk_array_add_string( buf, empty_default ); 
	} 
	splunk_json_add( name, buf ); 
	free(buf); 
	return; 
}

char *splunk_array_add_int( char *buffer, int value ) { 
	char	element[MAX_ELEMENT]; 

	snprintf(element, MAX_ELEMENT, "%d", value); 
	buffer = splunk_array_add_value( buffer, element ); 
	return(buffer); 
} 

char *splunk_array_add_float( char *buffer, float value ) { 
	char	element[MAX_ELEMENT]; 

	snprintf(element, MAX_ELEMENT, "%f", value); 
	buffer = splunk_array_add_value( buffer, element ); 
	return(buffer); 
} 

char *splunk_array_add_string( char *buffer, char *value ) { 
	char	element[MAX_ELEMENT]; 

	element[0] = '\0';
	if( value != NULL ) { 
		snprintf(element, MAX_ELEMENT, "\"%s\"", value); 
	}
	buffer = splunk_array_add_value( buffer, element ); 
	return(buffer); 
}

char *splunk_array_add_value( char *buffer, char *value ) { 
	int last_posn; 
	int i; 
	int ret; 

	size_t 	value_size; 
	size_t	buffer_size;
	char	element[MAX_ELEMENT]; 

	value_size = strlen(value); 
	if( value_size < 0 || value_size > ( MAX_ELEMENT - 100 )) { 
		printf("ERROR: splunk_array_add_value, value length is invalid: %d\n", (int)value_size); 
		if( buffer ) { free(buffer); }
		return(NULL); 
	}

	if( buffer == NULL ) { 
		ret = snprintf(element, MAX_ELEMENT, "[\n      %s\n    ]", value); 
		value_size = strlen(element); 

		buffer = (char *)calloc( value_size + 1, sizeof(char)); 		// Plus 1 for trailing zero
		memcpy( buffer, element, value_size + 1 ); 
	} else { 

		if( value_size ) { 

			last_posn = -1; 
			for( i = strlen( buffer ); i >= 0; i-- ) { 
				if( buffer[i] == '\n' ) { 
					last_posn = i; 
					i = 0; 
				}
			}
			if( last_posn <= 0 ) { 
				printf("ERROR: Unable to find last event in array buffer. Dropping: %s", value); 
				return( buffer ); 
			}
	
			buffer[ last_posn ] = '\0'; 					// truncate at CR
			ret = snprintf( element, MAX_ELEMENT, ",\n      %s\n    ]", value ); 

			buffer_size = strlen( buffer ) + strlen(element) + 1; 
	
			buffer = (char *)realloc( buffer, buffer_size );
			strncat( buffer, element, buffer_size );

		}
	}

#ifdef DEBUG
	splunk_json_dump( buffer ); 
#endif
	return(buffer); 
}

int splunk_json_add(char *name, char *value) { 
	char	element[ MAX_ELEMENT ]; 				// Nice sized temporary buffer.
	int 	element_length;
	int	last_posn; 
	int	last_counter; 
	int 	i;


	if( strlen( name ) + strlen( value ) > 900 ) { 			// don't even bother with long entries.

		if( strlen(name) > 10 ) { name[10] = '\0'; }				// truncate the bastards
		if( strlen(value) > 10 ) { value[10] = '\0'; } 		// truncate the bastards
	
		printf("ERROR: Item: %s...: %s..., is too long, ignoring.", name, value); 

		return(1); 
	}

	element_length = snprintf(element, MAX_ELEMENT, "    \"%s\": %s\n  }\n}", name, value);

#ifdef DEBUG
	printf("json buffer length %d before adding element of length %d\n", (int)strlen( splunk_json_buffer ), element_length);
#endif 

	if( element_length + strlen( splunk_json_buffer ) >= (MAX_JSON - 1)) {							// This isn't entirely accurate, but it should
		printf("ERROR: Appending Item: %s, (%s), will exceed maximum json buffer size, skipping\n", name, value);	// keep us from buffer overflow.
		return(1); 
	}

	last_posn = -1; last_counter = 0; 
	for( i = strlen( splunk_json_buffer ); i >= 0; i-- ) { 
		if( splunk_json_buffer[i] == '\n' ) { 
			last_counter++; 
			if( last_counter > 1 ) { 		// Want 2nd carriage return
				last_posn = i; 
				i = 0; 
			}
		}
	}

	if( last_posn <= 0 ) { 				// Both zero, and -1 would be bad....
		printf("ERROR: Unable to find last event in Splunk json buffer.  Reinitializing json, and dropping: %s: %s", name, value); 
		splunk_json_initialize(); 
		return(1); 
	}

	splunk_json_buffer[ last_posn ] = '\0'; 
	if( splunk_json_event_count ) { strncat( splunk_json_buffer, ",", 	MAX_JSON - strlen( splunk_json_buffer) - 1 ); } 
	strncat( splunk_json_buffer, "\n", 	MAX_JSON - strlen( splunk_json_buffer) - 1 ); 
	strncat( splunk_json_buffer, element, 	MAX_JSON - strlen( splunk_json_buffer) - 1 ); 

#ifdef DEBUG
	printf("json buffer after add: %d\n", (int)strlen( splunk_json_buffer ));
#endif 

	splunk_json_event_count++;

#ifdef DEBUG
	splunk_json_dump(splunk_json_buffer); 
#endif 
	return(0);
}

// Why, yes, we did appropriate this code from the libcurl examples.
static size_t splunk_post_response_callback(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
	char *ptr = (char *)realloc(mem->memory, mem->size + realsize + 1);
	if(!ptr) {
		/* out of memory! */ 
		printf("ERROR: not enough memory (realloc returned NULL)\n");
		return(0);
	}
 
	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;
 
	return(realsize);			// I wonder if this is really paid attention to.....
}
 
int splunk_json_send(void) {
	CURL *h_curl;
	CURLcode res;

	char	success_string[] = "{\"text\":\"Success\",\"code\":0}";
	char	authorization_token[1024]; 
	struct curl_slist *headers = NULL;

	struct MemoryStruct chunk;

	if( splunk_json_event_count == 0 ) { 
		printf("ERROR: splunk_json_send() called, with no events appended."); 
		return(1); 
	}
 
	chunk.memory = (char *)malloc(1);  	/* will be grown as needed by realloc above */ 
	chunk.size = 0;    			/* no data at this point */ 

	snprintf(authorization_token, 1024, "Authorization: Splunk %s", HEC_TOKEN); 
 
	curl_global_init(CURL_GLOBAL_ALL);
	h_curl = curl_easy_init();

	if(h_curl) {
		// Specify URL to send to.
		curl_easy_setopt(h_curl, CURLOPT_URL, HEC_URL);

		// Add authorization token
		headers = curl_slist_append(headers, authorization_token); 
		curl_easy_setopt(h_curl, CURLOPT_HTTPHEADER, headers);
 
		/* send all data received to this function  */ 
		curl_easy_setopt(h_curl, CURLOPT_WRITEFUNCTION, splunk_post_response_callback);
 
		/* we pass our 'chunk' struct to the callback function */ 
		curl_easy_setopt(h_curl, CURLOPT_WRITEDATA, (void *)&chunk);
 
		/* some servers don't like requests that are made without a user-agent field, so we provide one */ 
		curl_easy_setopt(h_curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

		// Data to send in POST 
		curl_easy_setopt(h_curl, CURLOPT_POSTFIELDS, splunk_json_buffer);
 
		/* Perform the request, res will get the return code */ 
		res = curl_easy_perform(h_curl);

		/* Check for errors */ 
		if(res != CURLE_OK) {
			printf("ERROR: Splunk curl_easy_perform() failed: %s, dropping complete event.\n", curl_easy_strerror(res));
		} else {
			/*
			* Now, our chunk.memory points to a memory block that is chunk.size
			* bytes big and contains the remote response.
			* {"text":"Success","code":0}
			*/ 
#ifdef DEBUG
			printf("POST RESULTS: %s\n", chunk.memory); 
#endif
			if( strncmp( chunk.memory, success_string, strlen( success_string )) != 0 ) { 
				printf("WARNING: Failed to post to Splunk HEC: %s, event lost\n", chunk.memory); 
			}
		}
 
		/* always cleanup */ 
		curl_slist_free_all(headers); 
		curl_easy_cleanup(h_curl);
 
		/* we're done with libcurl, so clean it up */ 
		curl_global_cleanup();
	} else { 
		printf("ERROR: libCURL easy_init failed, dropping complete json event..."); 
	}

	free(chunk.memory);
	splunk_json_initialize(); 
	return(0);
}


#ifdef TEST
int main(int argc, char **argv) { 
	char element[MAX_ELEMENT]; 

	char *buf = NULL; 

	splunk_json_initialize(); 

	log_var_int( 3 );
	log_var_double( 3.141529 ); 
	log_var_double( wall_time );
	log_var_str( "Wootabega!" ); 
	splunk_json_add("name1", "\"value1\""); 
	splunk_json_add("name2", "\"value2\""); 
	splunk_json_add("name3", "\"value3\""); 

//	buf = splunk_array_add_string( buf, "BufferArray1" ); 
//	buf = splunk_array_add_string( buf, "BufferArray2" ); 
//	buf = splunk_array_add_string( buf, "BufferArray3" ); 

//	splunk_json_array_begin( "minor_alarm_txt1" ); 
//	buf = splunk_array_add_string( buf, "alarm1" ); 
	buf = splunk_array_add_int( buf, 79 ); 
	buf = splunk_array_add_float( buf, 3.0 ); 
	// splunk_array_end( buf, "minor_alarm_text", "No minor_alarms found" ); 
	splunk_array_end( buf, "minor_alarm_text", NULL ); 
//
//	splunk_json_array_begin( "minor_alarm_txt2" ); 
//	splunk_json_array_end( NULL ); 

splunk_json_add("sat", 
 "[\n"
 "    {\n"
 "     \"prn\": 0,\n"
 "     \"azimuth\": \"0\",\n"
 "     \"disabled\": \"0\",\n"
 "     \"el_dir\": \"U\",\n"
 "     \"elevation\": \"123.4\",\n"
 "     \"level_msg\": \"0\",\n"
 "     \"sig_level\": \"100\",\n"
 "     \"tracking\": \"1\", \n"
 "     \"visible\": \"1\"\n"
 "   },\n"
 "   {\n"
 "     \"prn\": \"1\",\n"
 "     \"azimuth\": \"99\",\n"
 "     \"disabled\": \"0\",\n"
 "     \"el_dir\": \"U\",\n"
 "     \"elevation\": \"61\",\n"
 "     \"level_msg\": \"14\",\n"
 "     \"sig_level\": \"47\",\n"
 "     \"tracking\": \"1\", \n"
 "     \"visible\": \"1\"\n"
 "   }\n"
 "  ]" );

	splunk_json_add("name4", "\"value4\""); 

splunk_json_add("object", 
"  {\n"
"    \"a\": \"b\",\n"
"    \"c\": \"d\",\n"
"    \"e\": \"f\"\n"
"  }" );

	splunk_json_add("name5", "\"final\""); 

//	splunk_json_send(); 
	exit(1);
}
#endif
