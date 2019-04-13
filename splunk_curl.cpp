/* 
 * libcurl interface (eg: Mostly libcurl sample code) 
 *  
 */ 


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

#include "parson.h"
#include "splunk.h"

// Used for retrieving responses from via libCURL
struct MemoryStruct {
	char *memory;
	size_t size;
};

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

// Mostly this too...
int splunk_json_send(char *HEC_TOKEN, char *HEC_URL, JSON_Object *json_buffer) {

	CURL *h_curl;
	CURLcode res;

	char	success_string[] = "{\"text\":\"Success\",\"code\":0}";
	char	authorization_token[1024]; 
	struct 	curl_slist *headers = NULL;

	struct 	MemoryStruct chunk;
	int 	retval; 
	char	*serialized_string;

	serialized_string = json_serialize_to_string_pretty( json_object_get_wrapping_value( json_buffer ) );
	if( strlen(serialized_string) == 0 ) { 
		printf("ERROR: splunk_json_send() called, with no json text."); 
		return(1); 
	}
 
	chunk.memory = (char *)malloc(1);  	/* will be grown as needed by realloc above */ 
	chunk.size = 0;    			/* no data at this point */ 

	snprintf(authorization_token, 1024, "Authorization: Splunk %s", HEC_TOKEN); 
 
	curl_global_init(CURL_GLOBAL_ALL);
	h_curl = curl_easy_init();

	retval = 1; 				// Presume we failed.
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
		curl_easy_setopt(h_curl, CURLOPT_POSTFIELDS, serialized_string);
 
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
			} else { 
				retval = 0;
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

	json_free_serialized_string( serialized_string ); 
	free(chunk.memory);
	return(retval);
}

