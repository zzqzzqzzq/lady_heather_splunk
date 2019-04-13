/*
 * assorted splunk definitions
 * 
 */
#define MAX_ELEMENT     	4096
#define MAX_JSON		1048576		// Maximum JSON data POST size per default Splunk (1Mb)

/* 
 * splunk_curl.cpp 
 */ 
int splunk_json_send(char *HEC_TOKEN, char *HEC_URL, JSON_Object *json_buffer);

/* 
 * functions defined in splunk.cpp
 */ 

void splunk_log(void);

/* 
 * splunk_json.cpp 
 */ 

int  splunk_json_add(JSON_Object *obj, char *name, JSON_Value *value); 
JSON_Object *splunk_json_initialize(JSON_Object *buf);
void splunk_log_sat( JSON_Object *sat_object, int prn );
void splunk_json_dump(JSON_Object *buf);

/* 
 * Makes life easier in splunk_log()
 */

#define log_var_int(OBJ, VAR) 			splunk_json_add(OBJ, #VAR, json_value_init_number( (double) VAR )); 
#define log_var_uint(OBJ, VAR) 			splunk_json_add(OBJ, #VAR, json_value_init_number( (double) VAR ));
#define log_var_long(OBJ, VAR) 			splunk_json_add(OBJ, #VAR, json_value_init_number( (double) VAR ));
#define log_var_ulong(OBJ, VAR) 		splunk_json_add(OBJ, #VAR, json_value_init_number( (double) VAR ));
#define log_var_float(OBJ, VAR) 		splunk_json_add(OBJ, #VAR, json_value_init_number( (double) VAR ));
#define log_var_double(OBJ, VAR) 		splunk_json_add(OBJ, #VAR, json_value_init_number( (double) VAR ));

#define log_var_str(OBJ, VAR) 			splunk_json_add(OBJ, #VAR, json_value_init_string( VAR ));
#define log_var_conststr(OBJ, VAR) 		splunk_json_add(OBJ, #VAR, json_value_init_string( #VAR ));
#define log_var_funcstr(OBJ, VAR, FUNC) 	splunk_json_add(OBJ, #VAR, json_value_init_string( FUNC ));
