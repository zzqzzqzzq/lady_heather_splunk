/*
 * assorted splunk definitions
 * 
 */

#define MAX_ELEMENT     4096

/* 
 * functions defined in splunk.cpp
 */ 

void splunk_json_dump(void);
int splunk_json_initialize(void);
int splunk_json_add(char *name, char *value);
int splunk_json_send(void);
void splunk_log(void);

char *splunk_array_add_value( char *buf, char *value ); 
char *splunk_array_add_string( char *buf, char *value ); 
char *splunk_array_add_float( char *buf, float value ); 
char *splunk_array_add_int( char *buf, int value ); 
void splunk_array_end( char *buf, char *name, char *empty_default ); 

#define TYPE_STRING 	0x01
#define TYPE_NUMBER 	0x02
#define TYPE_OBJECT 	0x03
#define TYPE_ARRAY 	0x04

/* 
 * Makes life easier in splunk_log()
 */
#define log_var_int(VAR) 		snprintf( element, MAX_ELEMENT, "%d", VAR); 	 splunk_json_add(#VAR, element ); 
#define log_var_uint(VAR) 		snprintf( element, MAX_ELEMENT, "%u", VAR); 	 splunk_json_add(#VAR, element ); 
#define log_var_long(VAR) 		snprintf( element, MAX_ELEMENT, "%ld", VAR); 	 splunk_json_add(#VAR, element ); 
#define log_var_ulong(VAR) 		snprintf( element, MAX_ELEMENT, "%lu", VAR); 	 splunk_json_add(#VAR, element ); 
#define log_var_float(VAR) 		snprintf( element, MAX_ELEMENT, "%f", VAR); 	 splunk_json_add(#VAR, element ); 
#define log_var_double(VAR) 		snprintf( element, MAX_ELEMENT, "%lf", VAR); 	 splunk_json_add(#VAR, element ); 
#define log_var_str(VAR) 		snprintf( element, MAX_ELEMENT, "\"%s\"", VAR);  splunk_json_add(#VAR, element ); 
#define log_var_conststr(VAR) 		snprintf( element, MAX_ELEMENT, "\"%s\"", #VAR); splunk_json_add(#VAR, element ); 
#define log_var_funcstr(VAR, FUNC) 	snprintf( element, MAX_ELEMENT, "\"%s\"", FUNC); splunk_json_add(#VAR, element ); 
