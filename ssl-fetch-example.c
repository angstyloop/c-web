/*
COMPILE

    clear && gcc -o ssl-fetch-example ssl-fetch-example.c -lcurl && ./ssl-fetch-example

REFERENCES

    - https://curl.se/libcurl/c/https.html

LEGAL

    The cURL project has their own license inspired by MIT-X. They want you
    to add it in each bit of their source code you modify and use, even
    the examples. That's how they get ya! Just kidding, it's great FOSS. 

    The cURL project's custom license is below:

--------------------------------------------------------------------------------
COPYRIGHT AND PERMISSION NOTICE

Copyright (c) 1996 - 2022, Daniel Stenberg, <daniel@haxx.se>, and many
contributors, see the THANKS file.

All rights reserved.

Permission to use, copy, modify, and distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright
notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN
NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of a copyright holder shall not
be used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization of the copyright holder.
--------------------------------------------------------------------------------
*/

#include <stdlib.h>
#include <stdio.h>
#include <curl/curl.h>
 
////////////////////////////////////////////////////////////////////////////////
int main() {
    // Initialize cURL global resources.
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Initialize cURL local resources, and if successful, use it to make a
    // simply GET request.
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://example.com/");
 
#ifdef SKIP_PEER_VERIFICATION
     // If you want to connect to a site who is not using a certificate that is
     // signed by one of the certs in the CA bundle you have, you can skip the
     // verification of the server's certificate. This makes the connection
     // A LOT LESS SECURE.
     //
     // If you have a CA cert for the server stored someplace else than in the
     // default bundle, then the CURLOPT_CAPATH option might come handy for
     // you.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
 
#ifdef SKIP_HOSTNAME_VERIFICATION
    // If the site you are connecting to uses a different host name that what
    // they have mentioned in their server certificate's commonName (or
    // subjectAltName) fields, libcurl will refuse to connect. You can skip
    // this check, but this will make the connection less secure.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif
 
    // Send a GET request, printing the response to standard output, and save
    // the HTTP response code.
    CURLcode res = curl_easy_perform(curl);

    // Check the HTTP response code.
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        // Cleanup cURL local resources.
        curl_easy_cleanup(curl);
        exit(EXIT_FAILURE);
    }

    // Do more stuff
    // ...
 
    // Cleanup cURL local resources.
    curl_easy_cleanup(curl);
  }
 

  // Cleanup cURL global resources.
  curl_global_cleanup();
 
  return EXIT_SUCCESS;
}
